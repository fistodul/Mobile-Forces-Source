$modelname	"items\dogtagred.mdl"
$body mybody	"dogtagred.smd"
$staticprop
$surfaceprop	combine_metal
$cdmaterials	"models\items\dogtag"

$sequence idle	"dogtagred.smd"

$collisionmodel	"dogtagred.smd" { $concave }
$keyvalues
{
	prop_data
	{
		base			Metal.Medium 
		dmg.bullets		0 
		explosive_damage	100
		explosive_radius	50 
	}
}