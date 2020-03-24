gfx/menus/menu_buttonback_new
{
	nopicmip
	nomipmaps
    {
        map gfx/menus/menu_buttonback_new
        blendFunc add
    }
    {
        map gfx/hud/static_menur
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll -1 0
    }
    {
        map gfx/hud/static_menur
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 1 0
    }
    {
        map gfx/hud/static_menur
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll -1.3 0
    }
    {
        map gfx/hud/static_menur
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 1.3 0
    }
}

gfx/menus/menu_blendbox2_new
{
	nopicmip
	nomipmaps
    {
        map gfx/menus/menu_buttonback2_new
        blendFunc add
    }
    {
        map gfx/hud/static_menur
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 1 0
    }
    {
        map gfx/hud/static_menur
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll -1 0
    }
    {
        map gfx/hud/static_menur
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 1.7 0
    }
    {
        map gfx/hud/static_menur
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll -1.7 0
    }
}

gfx/mp/warp
{
    nopicmip
    nomipmaps
    {
        map gfx/2d/net
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}
