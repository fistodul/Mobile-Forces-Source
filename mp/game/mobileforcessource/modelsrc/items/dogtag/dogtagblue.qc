$modelname	"items\dogtagblue.mdl"
$body mybody	"dogtagblue.smd"
$staticprop
$surfaceprop	combine_metal
$cdmaterials	"models\items\dogtag"

$sequence idle	"dogtagblue.smd"

$collisionmodel	"dogtagblue.smd" { $concave }
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