$modelname	"items\dogtag.mdl"
$body mybody	"dogtag.smd"
$staticprop
$surfaceprop	combine_metal
$cdmaterials	"models\items\dogtag"

$sequence idle	"dogtag.smd"

$collisionmodel	"dogtag.smd" { $concave }
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