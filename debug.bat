@rem ---------------------------------------------
@rem
@rem Open code in VS, without needing to make project
@rem

@rem open exe in VS. Since code is compiled with debug info, 
@rem but we also need to add source code to place breakpoints

devenv exe\main.exe

@rem open source code in previously opened VS
@rem wait until VS is opened; otherwise first VS will not be reused

timeout /T 3
devenv /edit src\main.cpp