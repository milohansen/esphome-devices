# import os
# Import("env") # type: ignore

# print("************************************************")
# print("* Running extra_script_esphome.py")
# print("************************************************")

# print("Current CLI targets", COMMAND_LINE_TARGETS) # type: ignore
# print("Current Build targets", BUILD_TARGETS) # type: ignore

# all_entries = os.listdir('.')
# print("All entries:", all_entries)

with open('./src/lv_conf.h', 'a') as f:
    f.write('#define LV_COLOR_SCREEN_TRANSP 1\n')
    print("Appended LV_COLOR_SCREEN_TRANSP to lv_conf.h")
#     entire_file_content = f.read()
# print(entire_file_content)

# raise Exception("TERMINATING BUILD FROM extra_script_esphome.py FOR TESTING PURPOSES")