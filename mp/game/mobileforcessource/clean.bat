@echo off
rd "cache" "DownloadLists" "downloads" "lua_cache" "maps\graphs" "maps\soundcache" "materialsrc" "SAVE" "screenshots" /s /q
del "bin\client.pdb" "bin\server.pdb" "bin\gameui2.pdb" "cfg\config.cfg" "demoheader.tmp" "modelsounds.cache" "serverconfig.vdf" "stats.txt" "textwindow_temp.html" "voice_ban.dt" "mapsrc\*.bsp" "mapsrc\*.vmx" "mapsrc\*.lin" "mapsrc\*.log" "mapsrc\*.prt" /f /s /q
rd "..\..\..\dumps" /s /q
del "..\..\*.mdmp" /f /s
