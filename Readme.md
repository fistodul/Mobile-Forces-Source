**This is the source code of Mobile Forces Source xD (and apparently the MFS wiki)**
 
The folder structure is like this: 

###### Mobile-Forces-Source 
```
|>>>>game - the default game folder you would get with the source sdk 
|	|>>>>mobileforcessource 
|		|>>>>bin - the client.dll/dylib/so and server.dll/dylib/so go here after being compiled 
|		|>>>>mapsrc - map vmf's 
|		|>>>>modelsrc - model source files 
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
### SecobMod Source Code
```
Contains AI fixes and enhancements used in MFS
```
https://github.com/whoozzem/SecobMod
### Hyperborea Source Code
```
Cotains the implementation of gameui2 used in MFS, changes and improvements have been marked with MFS
```
Hyperborea (c) by Nicolas @ https://github.com/NicolasDe
https://github.com/SourceEnginePlayground/Hyperborea
This work is licensed under the Creative Commons Attribution 4.0 International License. 
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ 
Only our changes of Source SDK code are under the Creative Commons Attribution 4.0 International License.
### SOURCE SHADER EDITOR
Created by Biohazard https://github.com/Biohazard90/source-shader-editor  
Documentation on the editor itself can be found here:
http://developer.valvesoftware.com/wiki/Category:SourceShaderEditor
```
###               SOURCE 1 SDK LICENSE
https://github.com/WorldGamers/Mobile-Forces-Source/blob/master/LICENSE