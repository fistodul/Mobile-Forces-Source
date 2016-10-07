$modelname	"props\crate.mdl"
$body mybody	"crate.smd"
$staticprop
$surfaceprop	combine_metal
$cdmaterials	"models\props"

$sequence idle	"crate.smd"

$collisionmodel	"crate.smd" { $concave }
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