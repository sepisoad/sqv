[ui]
scale=1
font_path=/usr/local/share/fonts/SourceCodePro-Regular.ttf
font_size_interface=10
font_size_code=10
restore_watch_window=1
#layout=h(75,v(75,Source,Console),v(50,t(Commands,Locals,Watch,Struct,CmdSearch,Log,Exe),t(Files,Stack,Registers,Data,Thread,Breakpoints))))
layout=h(75,v(75,Source,Console),v(50,t(Commands,Locals,Watch,Struct,Exe),t(Stack,Thread,Breakpoints,Files,Data,CmdSearch))))

[executable]
path=.build/app_pak
arguments=
ask_directory=0

[gdb]
arguments=-ex "file .build/app_pak" -ex "source .breakpoints"

[commands]
START PROGRAM=file .build/app_pak; source .breakpoints; run&
SAVE BREAKPOINTS=save breakpoints .breakpoints;
LOAD BREAKPOINTS=source .breakpoints;


