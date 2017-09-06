"screen_basic.res"
{
	"Background"
	{
		"ControlName"	"MaterialImage"
		"fieldName"		"Background"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-2"
		"wide"			"480"
		"tall"			"240"
 
		"material"		"vgui/screens/vgui_bg"
	}
	"DismantleButton"
	{
		"ControlName"	"MaterialButton"
		"fieldName"		"Dismantle"
		"xpos"			"78"
		"ypos"			"160"
		"wide"			"324"
		"tall"			"48"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"2"
		"labelText"		"Dismantle"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Default"		"0"
		"command"		"1"
		"paintborder"	"0"
 
		"enabledImage"
		{
			"material"	"vgui/screens/vgui_button_enabled"
			"color" "255 255 255 255"
		}
 
		"mouseOverImage"
		{
			"material"	"vgui/screens/vgui_button_hover"
			"color" "255 255 255 255"
		}
 
		"pressedImage"
		{
			"material"	"vgui/screens/vgui_button_pushed"
			"color" "255 255 255 255"
		}
 
		"disabledImage"
		{
			"material"	"vgui/screens/vgui_button_disabled"
			"color" "255 255 255 255"
		}
	}
}
