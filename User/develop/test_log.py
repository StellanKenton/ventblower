# Author   : Stellan
# Copyright: Copyright (c) 2026 ResMed. All rights reserved.
"""Exercise the production log core and console with two fake links on the host."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / 'User/module/log'

TEST_HEADER = r'''
/************************************************************************************
* @file     : logtest.h
* @brief    : Host test doubles for the log module.
***********************************************************************************/
#ifndef LOGTEST_H
#define LOGTEST_H

#include <stdbool.h>
#include <stdint.h>

#include "log.h"

#define LOG_TEST_LINK_DATA_SIZE     8192U
#define LOG_TEST_LINK_INPUT_SIZE    1024U

typedef struct stLogTestLink {
    char data[LOG_TEST_LINK_DATA_SIZE];
    uint32_t length;
    uint32_t capacity;
    uint8_t input[LOG_TEST_LINK_INPUT_SIZE];
    uint32_t inputLength;
    uint32_t inputIndex;
    uint32_t pending;
    uint32_t initCount;
} stLogTestLink;

extern stLogTestLink gLogTestLinks[LOG_LINK_COUNT];
extern uint32_t gLogTestNowMs;
extern uint32_t gLogTestTimeStepMs;
extern bool gLogTestInIsr;

void logTestReset(void);
void logTestInput(uint8_t linkId, const char *text);
void logTestExpect(uint8_t linkId, const char *expected);

#endif
/**************************End of file********************************/
'''

TEST_PORT = r'''
/************************************************************************************
* @file     : logtestport.c
* @brief    : Fake port with two capturing links, a settable clock and an ISR flag.
***********************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "logtest.h"

stLogTestLink gLogTestLinks[LOG_LINK_COUNT];
uint32_t gLogTestNowMs;
uint32_t gLogTestTimeStepMs;
bool gLogTestInIsr;

/** Accept a whole line or nothing, like a real link with a full buffer. */
static uint32_t logTestWrite(stLogTestLink *link, const uint8_t *data, uint32_t length) {
    if ((link->length + length) > link->capacity) {
        return 0U;
    }
    memcpy(&link->data[link->length], data, length);
    link->length += length;
    link->data[link->length] = '\0';
    return length;
}
static uint32_t logTestRead(stLogTestLink *link, uint8_t *data, uint32_t length) {
    uint32_t lCount = link->inputLength - link->inputIndex;
    if (lCount > length) {
        lCount = length;
    }
    memcpy(data, &link->input[link->inputIndex], lCount);
    link->inputIndex += lCount;
    return lCount;
}
static void logTestInitA(void) { gLogTestLinks[0].initCount++; }
static void logTestInitB(void) { gLogTestLinks[1].initCount++; }
static uint32_t logTestWriteA(const uint8_t *data, uint32_t length) { return logTestWrite(&gLogTestLinks[0], data, length); }
static uint32_t logTestWriteB(const uint8_t *data, uint32_t length) { return logTestWrite(&gLogTestLinks[1], data, length); }
static uint32_t logTestReadA(uint8_t *data, uint32_t length) { return logTestRead(&gLogTestLinks[0], data, length); }
static uint32_t logTestReadB(uint8_t *data, uint32_t length) { return logTestRead(&gLogTestLinks[1], data, length); }
static uint32_t logTestPendingA(void) { return gLogTestLinks[0].pending; }
static uint32_t logTestPendingB(void) { return gLogTestLinks[1].pending; }

static const stLogPortOps gLogTestOps[LOG_LINK_COUNT] = {
    {"a", logTestInitA, logTestWriteA, logTestReadA, logTestPendingA},
    {"b", logTestInitB, logTestWriteB, logTestReadB, logTestPendingB},
};

const stLogPortOps *logPortGetOps(uint8_t linkId) {
    return (linkId < LOG_LINK_COUNT) ? &gLogTestOps[linkId] : NULL;
}
uint32_t logPortGetTimeMs(void) {
    uint32_t lNow = gLogTestNowMs;
    gLogTestNowMs += gLogTestTimeStepMs;
    return lNow;
}
bool logPortIsInIsr(void) { return gLogTestInIsr; }

void logTestReset(void) {
    for (uint8_t i = 0U; i < LOG_LINK_COUNT; i++) {
        uint32_t lInitCount = gLogTestLinks[i].initCount;
        memset(&gLogTestLinks[i], 0, sizeof(gLogTestLinks[i]));
        gLogTestLinks[i].capacity = LOG_TEST_LINK_DATA_SIZE - 1U;
        gLogTestLinks[i].initCount = lInitCount;
    }
    gLogTestNowMs = 1234U;
    gLogTestTimeStepMs = 0U;
    gLogTestInIsr = false;
}
void logTestInput(uint8_t linkId, const char *text) {
    stLogTestLink *lLink = &gLogTestLinks[linkId];
    size_t lLength = strlen(text);
    assert((lLink->inputLength + lLength) <= sizeof(lLink->input));
    memcpy(&lLink->input[lLink->inputLength], text, lLength);
    lLink->inputLength += (uint32_t)lLength;
}
void logTestExpect(uint8_t linkId, const char *expected) {
    if (strcmp(gLogTestLinks[linkId].data, expected) != 0) {
        fprintf(stderr, "link %u\nexpected: [%s]\nactual:   [%s]\n", (unsigned int)linkId, expected,
                gLogTestLinks[linkId].data);
        assert(0);
    }
}
/**************************End of file********************************/
'''

HARNESS = r'''
/************************************************************************************
* @file     : logtest.c
* @brief    : Verify line format, truncation, per-link drops, ISR guard and console input.
***********************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "console.h"
#include "logtest.h"

static const char gLogTestCsvHeader[] =
    "sequence,time_ms,air_x2,o2_x2,prox_x2,pinsp_x1,ppeep_x1,pexp_x1,ppat_x1,blower_x10,pref_x1,"
    "flowcomp_x1,pcorr_x1,effort_x1,ff_x1,vt_x10,vti_x10,vte_x10,target_x100,valve_x2,"
    "expiration_state,pressure_state,volume_pause,pause_settled,leak_lpm,flow_ref_lpm,"
    "flow_measurement_lpm,flow_effort,flow_blower_ff";
static uint32_t gLogTestEchoCalls;
static char gLogTestEchoArgs[CONSOLE_COMMAND_MAX_LENGTH];
static uint32_t gLogTestRebootCalls;
static char gLogTestNames[CONSOLE_REGISTERED_COMMAND_MAX][8];
static stConsoleCommand gLogTestFill[CONSOLE_REGISTERED_COMMAND_MAX];

static int8_t logTestEchoCommand(const char *arguments) {
    size_t lLength = strlen(arguments);
    assert(lLength < sizeof(gLogTestEchoArgs));
    memcpy(gLogTestEchoArgs, arguments, lLength + 1U);
    gLogTestEchoCalls++;
    return CONSOLE_STATUS_OK;
}
static int8_t logTestRebootCommand(const char *arguments) {
    (void)arguments;
    gLogTestRebootCalls++;
    return CONSOLE_STATUS_OK;
}
static int8_t logTestFailCommand(const char *arguments) {
    (void)arguments;
    return CONSOLE_STATUS_ERROR;
}
static const stConsoleCommand gLogTestCommands[] = {
    {"echo", "record arguments", logTestEchoCommand},
    {"reboot", "count calls", logTestRebootCommand},
    {"fail", "always fails", logTestFailCommand},
};
static const stConsoleCommand gLogTestDuplicate = {"echo", "same name", logTestEchoCommand};
static const stConsoleCommand gLogTestInvalid = {"bad", "no handler", NULL};

int main(void) {
    static char lLong[600];
    char lOverflow[CONSOLE_COMMAND_MAX_LENGTH + 16U];
    uint32_t lIndex;
    uint32_t lCalls;
    int8_t lStatus = CONSOLE_STATUS_OK;

    memset(lLong, 'x', sizeof(lLong) - 1U);
    logTestReset();
    logInit();
    assert((gLogTestLinks[0].initCount == 1U) && (gLogTestLinks[1].initCount == 1U));

    /* One formatted line reaches every link unchanged. */
    logTestReset();
    LOG_I("tag", "hello %d", 7);
    logTestExpect(0U, "[I][1234][tag] hello 7\r\n");
    logTestExpect(1U, "[I][1234][tag] hello 7\r\n");
    logTestReset();
    LOG_E("t", "e");
    LOG_W("t", "w");
    LOG_D("t", "d");
    LOG_R("abc");
    logWrite(LOG_LEVEL_INFO, NULL, "x");
    logTestExpect(0U, "[E][1234][t] e\r\n[W][1234][t] w\r\n[D][1234][t] d\r\nabc\r\n[I][1234][] x\r\n");

    /* Tagged lines stop at LOG_LINE_MAX_SIZE, raw lines at LOG_RAW_LINE_MAX_SIZE, both end with CRLF. */
    logTestReset();
    LOG_I("t", "%s", lLong);
    assert(gLogTestLinks[0].length == LOG_LINE_MAX_SIZE);
    assert(strncmp(gLogTestLinks[0].data, "[I][1234][t] xxx", 16U) == 0);
    assert(memcmp(&gLogTestLinks[0].data[LOG_LINE_MAX_SIZE - 2U], "\r\n", 2U) == 0);
    logTestReset();
    LOG_R("%s", lLong);
    assert(gLogTestLinks[0].length == LOG_RAW_LINE_MAX_SIZE);
    assert(memcmp(&gLogTestLinks[0].data[LOG_RAW_LINE_MAX_SIZE - 2U], "\r\n", 2U) == 0);

    /* The 300-character ventcore CSV header now survives intact. */
    assert(strlen(gLogTestCsvHeader) == 300U);
    logTestReset();
    LOG_R("%s", gLogTestCsvHeader);
    assert(gLogTestLinks[0].length == 302U);
    assert(memcmp(gLogTestLinks[0].data, gLogTestCsvHeader, 300U) == 0);

    /* A full link drops the whole line and counts it; the other link is unaffected. */
    logTestReset();
    gLogTestLinks[1].capacity = 5U;
    LOG_I("t", "drop me");
    logTestExpect(0U, "[I][1234][t] drop me\r\n");
    logTestExpect(1U, "");
    logTestReset();
    logShowStats();
    logTestExpect(0U, "[I][1234][log] link a dropped 0\r\n[I][1234][log] link b dropped 1\r\n"
                      "[I][1234][log] isr dropped 0\r\n");

    /* Calls from an ISR never reach a link. */
    logTestReset();
    gLogTestInIsr = true;
    LOG_E("t", "from isr");
    LOG_R("raw from isr");
    gLogTestInIsr = false;
    logTestExpect(0U, "");
    logTestExpect(1U, "");
    logShowStats();
    logTestExpect(0U, "[I][1234][log] link a dropped 0\r\n[I][1234][log] link b dropped 1\r\n"
                      "[I][1234][log] isr dropped 2\r\n");

    /* Registration statuses. */
    for (lIndex = 0U; lIndex < (sizeof(gLogTestCommands) / sizeof(gLogTestCommands[0])); lIndex++) {
        assert(consoleRegisterCommand(&gLogTestCommands[lIndex]) == CONSOLE_STATUS_OK);
    }
    assert(consoleRegisterCommand(&gLogTestCommands[0]) == CONSOLE_STATUS_OK);
    assert(consoleRegisterCommand(&gLogTestDuplicate) == CONSOLE_STATUS_DUPLICATE);
    assert(consoleRegisterCommand(&gLogTestInvalid) == CONSOLE_STATUS_INVALID_ARGUMENT);
    assert(consoleRegisterCommand(NULL) == CONSOLE_STATUS_INVALID_ARGUMENT);

    /* Dispatch trims spaces, reads in chunks and accepts CR, LF or CRLF. */
    logTestReset();
    logTestInput(0U, "  echo  hi there  \r\n");
    consoleProcess();
    assert((gLogTestEchoCalls == 1U) && (strcmp(gLogTestEchoArgs, "hi there") == 0));
    logTestInput(0U, "echo 0123456789012345678901234567890123456789\r");
    consoleProcess();
    assert(strcmp(gLogTestEchoArgs, "0123456789012345678901234567890123456789") == 0);
    logTestReset();
    logTestInput(0U, "foo bar\n");
    consoleProcess();
    logTestExpect(0U, "[W][1234][console] unknown command: foo bar\r\n");
    logTestReset();
    logTestInput(0U, "fail now\r\n");
    consoleProcess();
    logTestExpect(0U, "[W][1234][console] command failed: fail now\r\n");

    /* After an overflow the tail of the line is dropped instead of being run as a command. */
    memset(lOverflow, 'a', CONSOLE_COMMAND_MAX_LENGTH);
    strcpy(&lOverflow[CONSOLE_COMMAND_MAX_LENGTH], " reboot\r\n");
    logTestReset();
    logTestInput(0U, lOverflow);
    consoleProcess();
    assert(gLogTestRebootCalls == 0U);
    logTestExpect(0U, "[W][1234][console] command too long\r\n");
    logTestReset();
    logTestInput(0U, "rebooX\bt\r\n" "reboo" "Y\x7f" "t\r\n");
    consoleProcess();
    assert(gLogTestRebootCalls == 2U);

    /* Only CONSOLE_INPUT_LINK is read. */
    logTestReset();
    lCalls = gLogTestEchoCalls;
    logTestInput(1U, "echo other link\r\n");
    consoleProcess();
    assert(gLogTestEchoCalls == lCalls);

    /* consoleDrain waits on the console link only and gives up at the timeout. */
    logTestReset();
    gLogTestTimeStepMs = 1U;
    gLogTestLinks[0].pending = 100U;
    assert(consoleDrain(50U, 10U) == CONSOLE_STATUS_TIMEOUT);
    gLogTestLinks[0].pending = 40U;
    assert(consoleDrain(50U, 10U) == CONSOLE_STATUS_OK);
    gLogTestLinks[0].pending = 0U;
    gLogTestLinks[1].pending = 1000U;
    assert(consoleDrain(0U, 10U) == CONSOLE_STATUS_OK);

    /* The table reports FULL once every slot is used. */
    for (lIndex = 0U; (lIndex < CONSOLE_REGISTERED_COMMAND_MAX) && (lStatus == CONSOLE_STATUS_OK); lIndex++) {
        (void)snprintf(gLogTestNames[lIndex], sizeof(gLogTestNames[lIndex]), "c%02u", (unsigned int)lIndex);
        gLogTestFill[lIndex].name = gLogTestNames[lIndex];
        gLogTestFill[lIndex].description = "fill";
        gLogTestFill[lIndex].handler = logTestFailCommand;
        lStatus = consoleRegisterCommand(&gLogTestFill[lIndex]);
    }
    assert(lStatus == CONSOLE_STATUS_FULL);
    assert(lIndex == (CONSOLE_REGISTERED_COMMAND_MAX - 2U));
    return 0;
}
/**************************End of file********************************/
'''

LEVEL_HARNESS = r'''
/************************************************************************************
* @file     : logtestlevel.c
* @brief    : Verify the compile-time level filter and console echo.
***********************************************************************************/
#include <assert.h>
#include "console.h"
#include "logtest.h"

int main(void) {
    int lEvaluated = 0;

    /* Filtered levels compile to nothing, so their arguments are not even evaluated. */
    logTestReset();
    LOG_I("t", "%d", lEvaluated++);
    LOG_D("t", "%d", lEvaluated++);
    assert(lEvaluated == 0);
    logTestExpect(0U, "");
    LOG_W("t", "w");
    LOG_E("t", "e");
    logTestExpect(0U, "[W][1234][t] w\r\n[E][1234][t] e\r\n");

    /* Echo goes back to the console link only and CRLF ends one line. */
    logTestReset();
    logTestInput(0U, "xy\bz\r\n");
    consoleProcess();
    logTestExpect(0U, "xy\b \bz\r\n[W][1234][console] unknown command: xz\r\n");
    logTestExpect(1U, "[W][1234][console] unknown command: xz\r\n");
    return 0;
}
/**************************End of file********************************/
'''


def main():
    compiler = os.environ.get('CC') or shutil.which('gcc') or shutil.which('clang')
    if not compiler:
        compiler = next((str(p) for p in (
            Path('C:/msys64/ucrt64/bin/gcc.exe'),
            Path('C:/Qt/Tools/mingw1310_64/bin/gcc.exe')) if p.is_file()), None)
    if not compiler:
        raise SystemExit('Set CC to a native GCC or Clang compiler')
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment['PATH']
    with tempfile.TemporaryDirectory(prefix='vent-log-') as directory:
        work = Path(directory)
        (work / 'logtest.h').write_text(TEST_HEADER, encoding='utf-8', newline='\n')
        (work / 'logtestport.c').write_text(TEST_PORT, encoding='utf-8', newline='\n')
        (work / 'logtest.c').write_text(HARNESS, encoding='utf-8', newline='\n')
        (work / 'logtestlevel.c').write_text(LEVEL_HARNESS, encoding='utf-8', newline='\n')
        sources = [str(work / 'logtestport.c'), str(MODULE / 'log.c'), str(MODULE / 'console.c')]
        builds = (('logtest', [], 'logtest.c'),
                  ('logtestlevel', ['-DLOG_COMPILED_LEVEL=2', '-DCONSOLE_ECHO_ENABLE=1'], 'logtestlevel.c'))
        for name, defines, harness in builds:
            executable = work / f'{name}.exe'
            subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', *defines,
                            f'-I{work}', f'-I{MODULE}', str(work / harness), *sources,
                            '-o', str(executable)], check=True, env=environment)
            subprocess.run([str(executable)], check=True, env=environment)
    print('PASS: line format, 256/384 truncation, 300-char CSV header, per-link whole-line drops, '
          'ISR guard, console trim/chunk/CRLF, overflow discard, backspace, input link, drain timeout, '
          'registration statuses, compile-time level filter, echo')


if __name__ == '__main__':
    main()
