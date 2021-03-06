MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(patsubst %/,%,$(dir ${MKFILE_PATH}))
BUILD_DIR := ${CURRENT_DIR}/tmp

SRC:=BluePill_TFT16K_Mandelbrot.ino
BIN:=tmp/${SRC}.bin

# CONFIG_MAPLE_MINI_NO_DISABLE_DEBUG

BASE:=/usr/share/arduino
USER_BASE:=$(HOME)/.arduino15
USER_LIBS:=$(HOME)/Arduino/libraries
MAPLE:=$(USER_BASE)/packages/stm32duino/tools/stm32tools/2018.4.26/linux
BOARD:=stm32duino:STM32F1:genericSTM32F103C:device_variant=STM32F103CB,upload_method=DFUUploadMethod,cpu_speed=speed_72mhz,opt=osstd -vid-pid=0X1EAF_0X0004
# -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.arm-none-eabi-gcc.path=/home/ttsiod/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1 -prefs=runtime.tools.stm32tools.path=/home/ttsiod/.arduino15/packages/stm32duino/tools/stm32tools/2018.4.2

HARDWARE:=-hardware ${BASE}/hardware -hardware ${USER_BASE}/packages 
TOOLS:=-tools ${BASE}/tools-builder -tools ${USER_BASE}/packages
LIBRARIES=-built-in-libraries ${BASE}/lib
LIBRARIES+=-libraries ${USER_LIBS}  # Where U8g2 comes from
WARNINGS:=-warnings all -logger human

ARDUINO_BUILDER_OPTS=${HARDWARE} ${TOOLS} ${LIBRARIES}
ARDUINO_BUILDER_OPTS+=-fqbn=${BOARD} ${WARNINGS}
ARDUINO_BUILDER_OPTS+=-verbose -build-path ${BUILD_DIR} 

all:	small_screen

fast:
	echo '#define ENTIRE_SCREEN false' > config.h

slow:
	echo '#define ENTIRE_SCREEN true' > config.h

small_screen:	fast common

big_screen:	slow common

common:	config.h
	@mkdir -p ${BUILD_DIR}
	arduino-builder -compile ${ARDUINO_BUILDER_OPTS} ${SRC} >build.log 2>&1 || grep error build.log
	grep warning build.log | grep ${SRC} || exit 0

clean:
	rm -rf ${BUILD_DIR} build.log .gdb_history

upload:
	TERM=xterm ~/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-gdb ./tmp/${SRC}.elf -x gdb.cmd

flash:
	TERM=xterm ~/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-gdb ./tmp/${SRC}.elf -x flash.cmd

openocd:
	openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
