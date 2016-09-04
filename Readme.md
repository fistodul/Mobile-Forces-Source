**This is the source code of Mobile Forces Source xD (and apparently the MFS wiki)**
 
The folder structure is like this: 

###### Mobile-Forces-Source 
```
|>>>>game - the default game folder you would get with the source sdk 
|	|>>>>mobileforcessource 
|		|>>>>bin - the client.dll/dylib/so and server.dll/dylib/so go here after being compiled 
|>>>>src - contains the solutions when they are generated alongside various engine things lol 
	|>>>>game - most of the code which will show up in-game and decide how shit works 
	|	|>>>>client - contains client-side files 
	|	|	|>>>>hl2dm - client-side hl2dm code, multiplayer base 
	|	|>>>>server - contains server-side files 
	|	|	|>>>>hl2dm - server-side hl2dm code, multiplayer base 
	|	|>>>>shared(has files that are shared betwean the client and server, lots of voodoo 
	|		|>>>>hl2dm(shared hl2dm code, the voodoo that that comes with the multiplayer base 
	|>>>>materialsystem 
	|	|>>>>stdshaders 
	|		|>>>>buildmfsshaders.bat - Used to compile shaders(edit it to the correct paths) 
	|>>>>createallprojects/.bat - Used to create the game solution 
	|>>>>creategameprojects/.bat - Used to create the solution for everything that can be modded 
```
there are lots of more folders but im lazy to write now :P 

for the time being i dont know how to make vpc create projects that are in the release config by default so change them 
from Debug to Release lol 
 
If you're looking for the mfs story... check out mfs.txt but that hasn't been updated in ages and prolly will change :P
Feel free to make any changes that could make the game better ^^ 
 
Text Images xD: 
```
			 . 
			... 
			... 
			... 
			... 
			... 
			... 
			... 
			... 
			... 
			... 
			... 
			... 
			... 
			... 
	..................... 
	.	World Gamers	. 
	..................... 
			... 
			... 
			... 
```

### Project-9 Source Code
```
https://github.com/NicolasDe/project-9
This work is licensed under the Creative Commons Attribution 4.0 International License. 
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ 
```
Only our changes of Source SDK code are under the Creative Commons Attribution 4.0 International License.  
Source SDK License: https://github.com/ValveSoftware/source-sdk-2013/blob/master/LICENSE 
### SOURCE SHADER EDITOR
Created by Biohazard https://github.com/Biohazard90/source-shader-editor  
Documentation on the editor itself can be found here:
http://developer.valvesoftware.com/wiki/Category:SourceShaderEditor
###               SOURCE 1 SDK LICENSE

Source SDK Copyright(c) Valve Corp.  

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE 
CORPORATION ("Valve").  PLEASE READ IT BEFORE DOWNLOADING OR USING 
THE SOURCE ENGINE SDK ("SDK"). BY DOWNLOADING AND/OR USING THE 
SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO 
THE TERMS OF THIS LICENSE PLEASE DON’T DOWNLOAD OR USE THE SDK.  

  You may, free of charge, download and use the SDK to develop a modified Valve game 
running on the Source engine.  You may distribute your modified Valve game in source and 
object code form, but only for free. Terms of use for Valve games are found in the Steam 
Subscriber Agreement located here: http://store.steampowered.com/subscriber_agreement/ 

  You may copy, modify, and distribute the SDK and any modifications you make to the 
SDK in source and object code form, but only for free.  Any distribution of this SDK must 
include this LICENSE file and thirdpartylegalnotices.txt.  
 
  Any distribution of the SDK or a substantial portion of the SDK must include the above 
copyright notice and the following: 

    DISCLAIMER OF WARRANTIES.  THE SOURCE SDK AND ANY 
    OTHER MATERIAL DOWNLOADED BY LICENSEE IS PROVIDED 
    "AS IS".  VALVE AND ITS SUPPLIERS DISCLAIM ALL 
    WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS 
    OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED 
    WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, 
    TITLE AND FITNESS FOR A PARTICULAR PURPOSE.  

    LIMITATION OF LIABILITY.  IN NO EVENT SHALL VALVE OR 
    ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, 
    INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER 
    (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF 
    BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF 
    BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) 
    ARISING OUT OF THE USE OF OR INABILITY TO USE THE 
    ENGINE AND/OR THE SDK, EVEN IF VALVE HAS BEEN 
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
 
       
If you would like to use the SDK for a commercial purpose, please contact Valve at 
sourceengine@valvesoftware.com.