################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"D:/TI/ccs1000/ccs/tools/compiler/ti-cgt-msp430_20.2.1.LTS/bin/cl430" -vmsp --use_hw_mpy=none --include_path="D:/TI/ccs1000/ccs/ccs_base/msp430/include" --include_path="C:/Users/Hieu/Desktop/CCS_v10/MSP43_SPI_LoRa" --include_path="D:/TI/ccs1000/ccs/tools/compiler/ti-cgt-msp430_20.2.1.LTS/include" --advice:power=all --define=__MSP430G2553__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


