
math.clamp = function(v, min, max)
	return math.min(max, math.max(v,min))
end

function nosewheel_info()
	return {
		lod_pixels={20,75,0},
		bounding_radius = 7,
		materials={'leg','tyre'}
	}
end
function nosewheel_static(lod)
	set_material('leg', .5, .5, .5, 1, .5, .5, .5, 2.0, 0, 0, 0)
	set_material('tyre', .3, .3, .3, 1, 0,0,0, 1, 0, 0, 0)
	use_material('leg')
	local v6 = v(0, 0, 0)
	local v7 = v(0, 3, 0)
	local v8 = v(0, 5, 0)
	local divs = lod*4
	cylinder(divs, v6, v8, v(0,0,1), .4)
	cylinder(divs, v7, v8, v(0,0,1), .5)
	use_material('tyre')
	xref_cylinder(divs, v(.5,5,0), v(1,5,0), v(0,0,1), 1.0)
end

function nosewheelunit_info()
	return {
		bounding_radius = 7,
		materials={'inside', 'matvar2'}
	}
end
function nosewheelunit_static(lod)
	set_material('inside', .2,.2,.2,1, 0,0,0, 1, 0,0,0)
end
function nosewheelunit_dynamic(lod)
	-- flaps
	local v6 = v(1.5, 0, 6)
	local v7 = v(1.5, 0, -1)
	local v8 = v(0, 0, 6)
	local v9 = v(0, 0, -1)
	set_material('matvar2', get_arg_material(2))

	use_material('inside')
	zbias(1, v(0,0,0), v(0,1,0))
	-- flap internal
	xref_quad(v8, v6, v7, v9)
	-- SHould use parameter material(2) here but param materials not done yet
	use_material('matvar2')
	local flap_ang = 0.5*math.pi*math.clamp(3*get_arg(0),0,1)
	local wheel_ang = 0.5*math.pi*math.clamp(1.5*(get_arg(0)-0.34), 0, 1)
	local vrot = 1.5*v(-math.cos(flap_ang), math.sin(flap_ang), 0)
	xref_quad(v7, v6, v6+vrot, v7+vrot)
	xref_quad(v7, v7+vrot, v6+vrot, v6)

	call_model('nosewheel', v(0,0,0), v(1,0,0),
	v(0,math.sin(wheel_ang),-math.cos(wheel_ang)), 1.0)
	zbias(0)
end

function mainwheel_info()
	return {
		lod_pixels = {50,100,0},
		bounding_radius = 8,
		materials = {'leg', 'tyre'}
	}
end
function mainwheel_static(lod)
	local v6 = v(0,0,0)
	local v7 = v(0,3,0)
	local v8 = v(0,5,0)
	-- crossbar
	local v13 = v(0, 5, 1.4)
	local v14 = v(0, 5, -1.4)
	local divs = 4*lod
	set_material('leg', .5,.5,.5,1, 1,1,1, 2, 0,0,0)
	use_material('leg')
	cylinder(divs, v6, v8, v(0,0,1), .4)
	cylinder(divs, v7, v8, v(0,0,1), .5)
	cylinder(4, v13, v14, v(1,0,0), .5)
	set_material('tyre', .3,.3,.3,1, 0,0,0, 1, 0,0,0)
	use_material('tyre')
	xref_cylinder(divs, v(.5, 5, 1.1), v(1, 5, 1.1), v(0,0,1), 1)
	xref_cylinder(divs, v(.5, 5, -1.1), v(1, 5, -1.1), v(0,0,1), 1)
end


function mainwheelunit_info()
	return {
		bounding_radius = 7,
		materials={'inside','matvar2'}
	}
end
function mainwheelunit_static(lod)
	set_material('inside', .2,.2,.2,1, 0,0,0, 1, 0,0,0)
end
function mainwheelunit_dynamic(lod)
	-- flaps
	local v6 = v(1.5, 0, 6)
	local v7 = v(1.5, 0, -1)
	local v8 = v(0, 0, 6)
	local v9 = v(0, 0, -1)
	set_material('matvar2', get_arg_material(2))

	use_material('inside')
	zbias(1, v(0,0,0), v(0,1,0))
	-- flap internal
	xref_quad(v8, v6, v7, v9)
	-- SHould use parameter material(2) here but param materials not done yet
	use_material('matvar2')
	local flap_ang = 0.5*math.pi*math.clamp(3*get_arg(0),0,1)
	local wheel_ang = 0.5*math.pi*math.clamp(1.5*(get_arg(0)-0.34), 0, 1)
	local vrot = 1.5*v(-math.cos(flap_ang), math.sin(flap_ang), 0)
	xref_quad(v7, v6, v6+vrot, v7+vrot)
	xref_quad(v7, v7+vrot, v6+vrot, v6)

	call_model('mainwheel', v(0,0,0), v(1,0,0),
	v(0,math.sin(wheel_ang),-math.cos(wheel_ang)), 1.0)
	zbias(0)
end

function ladybird_info()
	return {
		lod_pixels = {50,100,200,0},
		bounding_radius = 35,
		materials={'white','engines','matvar0', 'matvar2',
		'engine_inside'}
	}
end

function ladybird_static(lod)

	local v06 = v(-4.0, -5.0, -20.0);
	local v07 = v(4.0, -5.0, -20.0);

	local v08 = v(-6.0, 4.0, -10.0);
	local v09 = v(6.0, 4.0, -10.0);

	local v10 = v(-14.0, -5.0, -10.0);
	local v11 = v(-10.0, 5.0, 10.0);

	local v29 = v(10.0, 5.0, 10.0);
	local v30 = v(14.0, -5.0, -10.0);
	local v31 = v(-10.0, -5.0, 10.0);
	local v32 = v(10.0, -5.0, 10.0);

	local v33 = v(-12.0, 0.0, 10.0);
	local v34 = v(-12.0, 0.0, 13.0);

	--// thruster jets
	local v38 = v(-12.0, 0.0, 13.0);
	local v39 = v(-15.0, -3.0, -9.0);

	local v40 = v(-30.0, -4.0, 9.0);
	local v41 = v(-29.0, -5.5, 9.0);
	local v42 = v(-29.0, -4.0, 9.0);
	local v43 = v(-10.0, 0.0, -11.0);

	xref_thruster(v38, v(0,0,1), 50, true)
	xref_thruster(v43, v(0,0,-1), 25)
	
	set_material('white',.5,.5,.5,1,1,1,1,100)
	use_material('matvar0')
	-- matvar(0)
	quad(v06,v08,v09,v07)
	quad(v09,v08,v11,v29)
	xref_tri(v08,v06,v10)

	local divs = lod*2

	local wingtip_rear = v(30,-5,10)
	local cpoint_rear = v(20,4,10)

	local leadingedge_mid = v(24,-5,-3)
	local tmp = v(5,0,5)
	local cpoint_leadingedge1 = leadingedge_mid - tmp
	local cpoint_leadingedge2 = leadingedge_mid + tmp


	-- body flat side piece
	local normal = ((v29-v09):cross(v30-v09)):norm()
	local cpoint_bodycurve = 0.5*(v29+v30) + 3.0*(v29-v30):cross(normal):norm()
	xref_flat(divs, normal,
		{ v09 },
		{ v29 },
		{ cpoint_bodycurve, v30 }
		)

	-- top wing bulge
	xref_bezier_3x3(divs,divs,
		wingtip_rear, cpoint_leadingedge2, leadingedge_mid,
		cpoint_rear, v(17,5,0), cpoint_leadingedge1,
		v29, cpoint_bodycurve, v30)


	-- rear
	xref_flat(divs, v(0,0,1),
		{ wingtip_rear },
		{ cpoint_rear, v29 },
		{ v32 }
	)
	quad(v29,v11,v31,v32) -- rear
	use_material('matvar2')
	quad(v10,v06,v07,v30)
	quad(v32,v31,v10,v30)
	-- underside of wing
	xref_tri(v30, wingtip_rear, v32)
	-- wing leading edge underside
	xref_flat(divs, v(0,-1,0),
		{ v30 },
		{ cpoint_leadingedge1, leadingedge_mid },
		{ cpoint_leadingedge2, wingtip_rear }
	)

	zbias(1, v33, v(0,0,1))
	set_material('engines',.3,.3,.3,1,.3,.3,.3,20)
	use_material('engines')
	xref_tube(12, v33, v34, v(0,1,0), 2.5, 3.0)
	use_material('engine_inside')
	-- matanim!!
	xref_circle(12, v33, v(0,0,1), v(0,1,0), 2.5)
	-- wheels my friend
	
end
function lerp_materials(a, m1, m2)
	local out = {}
	a = math.fmod(a, 2.002)
	if a > 1.0 then a = 2.002 - a end
	local b = 1.0 - a
	for i = 1,11 do
		out[i] = a*m2[i] + b*m1[i]
	end
	return out
end
function ladybird_dynamic(lod)
	set_material('matvar0', get_arg_material(0))
	set_material('matvar2', get_arg_material(2))
	set_material('engine_inside', lerp_materials(get_arg(2)*30.0, {0, 0, 0, 1, 0, 0, 0, 10, .5, .5, 1 },
				{0, 0, 0, 1, 0, 0, 0, 10, 0, 0, .5 }))
	if get_arg(0) ~= 0 then
		local v35 = v(0.0, -5.0, -13.0);
		local v36 = v(-15.0, -5.0, 3.0);
		local v37 = v(15.0, -5.0, 3.0);

		zbias(1, v35, v(0,-1,0))
		call_model('nosewheelunit', v35, v(-1,0,0), v(0,-1,0), 1)
		call_model('mainwheelunit', v36, v(-1,0,0), v(0,-1,0), 1)
		call_model('mainwheelunit', v37, v(-1,0,0), v(0,-1,0), 1)
		zbias(0)
	end

end

function building1_info()
	return {
		bounding_radius = 10,
		materials={'mat'}
	}
end
function building1_static(lod)
	set_material("mat", .5, .5, .5, 1)
	use_material("mat")
	extrusion(v(0,0,-7.5), v(0,0,7.5), v(0,1,0), 6.0,
		v(-1,0,0), v(1,0,0), v(1,1,0), v(-1,1,0))
end

function building2_info()
	return {
		bounding_radius = 25,
		materials={'mat'}
	}
end
function building2_static(lod)
	set_material("mat", .5, .5, .5, 1)
	use_material("mat")
	extrusion(v(0,0,-16), v(0,0,16), v(0,1,0), 1.0,
		v(-16,0,0), v(16,0,0), v(16,20,0), v(-16,20,0))
end

function skyscraper1_info()
	return {
		bounding_radius=200,
		materials={'gray1', 'gray2'}
	}
end
function skyscraper1_static(lod)
	-- Inset black bit
	-- h g
	--
	-- e f
	-- d c
	-- a b
	local a = v(-20,0,10)
	local b = v(20,0,10)
	local c = v(20,40,10)
	local d = v(-20,40,10)
	local e = v(-20,48,10)
	local f = v(20,48,10)
	local g = v(20,200,10)
	local h = v(-20,200,10)
	set_material("gray1", .2,.2,.2,1, .7,.7,.7, 10)
	use_material("gray1")
	quad(a,b,c,d)
	quad(e,f,g,h)
	-- Outset
	-- h2 g2
	--
	-- e2 f2
	-- d2 c2
	-- a2 b2
	local a2 = v(-20,0,15)
	local b2 = v(20,0,15)
	local c2 = v(20,40,15)
	local d2 = v(-20,40,15)
	local e2 = v(-20,48,15)
	local f2 = v(20,48,15)
	local g2 = v(20,200,15)
	local h2 = v(-20,200,15)
	set_material("gray2", .9,.9,.9,1)
	use_material("gray2")
	-- inset bits
	quad(b,b2,c2,c)
	quad(d,c,c2,d2)
	quad(a,d,d2,a2)
	quad(f,e,e2,f2)
	quad(g,f,f2,g2)
	quad(h,g,g2,h2)
	quad(e,h,h2,e2)
	-- front face
	-- fd fc
	-- fa fb
	local fa = v(-30,0,15)
	local fb = v(30,0,15)
	local fc = v(30,210,15)
	local fd = v(-30,210,15)
	quad(d2,c2,f2,e2)
	quad(fc,fd,h2,g2)
	
	-- tri fan on front face
	xref_tri(g2,f2,c2)
	xref_tri(g2,c2,b2)
	xref_tri(g2,b2,fb)
	xref_tri(g2,fb,fc)

	top = v(0,260,-40)
	-- rear
	xref_quad(fc,fb,v(0,0,-40),v(0,210,-40))
	xref_tri(fc,v(0,210,-40),top)
	-- spire
	spire = v(0,300,-40)
	xref_tri(v(1.5,257.5,-37.25),
		top,spire)
	tri(v(-1.5,257.5,-37.25),v(1.5,257.5,-37.25),
		spire)
	-- front roof triangle section
	--      top
	--      tc
	--
	--   ta   tb
	-- fd       fc
	local ta = v(-21, 215, 9.5)
	local tb = v(21, 215, 9.5)
	local tc = v(0, 250, -29)
	quad(fd,fc,tb,ta)
	xref_quad(fc,top,tc,tb)
	use_material("gray1")
	tri(ta,tb,tc)
end

function clockhand_info()
	return {
		bounding_radius = 1,
		materials={'mat'}
	}
end
function clockhand_static(lod)
	set_material("mat",0,0,1,1, 0,0,0, 10)
	use_material("mat")
	zbias(3, v(0,0,0), v(0,0,1))
	tri(v(-0.06, -0.06, 0), v(0.06, -0.06, 0), v(0, 1, 0))
end

function clock_info()
	return {
		bounding_radius=1,
		materials={'face','numbers'}
	}
end
function clock_static(lod)
	set_material("face", 1,1,1,1)
	set_material("numbers",.5,.5,0,1)
	use_material("face")
	zbias(1, v(0,0,0), v(0,0,1))
	circle(24, v(0,0,0), v(0,0,1), v(0,1,0), 1.0)
	zbias(2, v(0,0,0), v(0,0,1))

	use_material("numbers")
	text("12", v(0,0.85,0), v(0,0,1), v(1,0,0), 0.15)
	text("1", v(0.425,0.74,0), v(0,0,1), v(1,0,0), 0.15)
	text("2", v(0.74,0.43,0), v(0,0,1), v(1,0,0), 0.15)
	text("3", v(0.85,0,0), v(0,0,1), v(1,0,0), 0.15)
	text("4", v(0.74,-.425,0), v(0,0,1), v(1,0,0), 0.15)
	text("5", v(0.425,-.74,0), v(0,0,1), v(1,0,0), 0.15)
	text("6", v(0,-.85,0), v(0,0,1), v(1,0,0), 0.15)
	text("7", v(-.425,-.736,0), v(0,0,1), v(1,0,0), 0.15)
	text("8", v(-.74,-.425,0), v(0,0,1), v(1,0,0), 0.15)
	text("9", v(-.85,0,0), v(0,0,1), v(1,0,0), 0.15)
	text("10", v(-.74,.425,0), v(0,0,1), v(1,0,0), 0.15)
	text("11", v(-.425,.736,0), v(0,0,1), v(1,0,0), 0.15)

end
function clock_dynamic(lod)
	local t = os.time()
	local handPos = (t%60)/60

	call_model("clockhand", v(0,0,0),
			v(math.cos(-handPos),-math.sin(-handPos),0),
			v(-math.sin(-handPos), -math.cos(-handPos),0), 0.65)
	local handPos = (t%(60*60))/(60*60)
	call_model("clockhand", v(0,0,0),
			v(math.cos(-handPos),-math.sin(-handPos),0),
			v(-math.sin(-handPos), -math.cos(-handPos),0), 0.5)
end
function church_info()
	return {
		bounding_radius=45,
		materials={'body','spire'}
	}
end
function church_static(lod)
	set_material("body", .5, .5, .3,1)
	use_material("body")
	extrusion(v(0,0,-18), v(0,0,12), v(0,1,0), 1.0,
		v(-7,0,0), v(7,0,0), v(7,10,0), v(-7,10,0))
	extrusion(v(0,0,-18), v(0,0,-4), v(0,1,0), 1.0,
		v(-7,10,0), v(7,10,0), v(7,25,0), v(-7,25,0))
	local roof1 = v(0,20,12)
	local roof2 = v(0,20,-4)
	local spire = v(0,45,-11)
	tri(v(-7,10,12), v(7,10,12), roof1)
	
	set_material("spire", .8, .3, .0,1)
	use_material("spire")
	xref_quad(roof2, roof1, v(7,10,12), v(7,10,-4))
	xref_tri(spire, v(7,25,-4), v(7,25,-18))
	tri(spire, v(-7,25,-4), v(7,25,-4))
	tri(spire, v(7,25,-18), v(-7,25,-18))

	local clockpos1 = v(-7,18,-11)
	local clockpos2 = v(7,18,-11)
	
	call_model("clock", clockpos1, v(0,0,1), v(0,1,0), 2.5)
	call_model("clock", clockpos2, v(0,0,-1), v(0,1,0), 2.5)
end

function towerOfShit_info()
	return {
		bounding_radius=20,
		materials={'mat1'}
	}
end
function towerOfShit_static(lod)
	set_material("mat1", 1,1,1,1)
	use_material("mat1")
	for i = 0,20 do
		local _xoff = 10*noise(v(0,i*10,0))
		local _zoff = 10*noise(v(0,0,i*10))
		local _start = v(30+_xoff,i*10,_zoff)
		local _end = v(30+_xoff,10+i*10,_zoff)
		xref_cylinder(16, _start, _end, v(1,0,0), 10.0+math.abs(10*noise(0,0.1*i,0)))
	end
end

function boringHighRise_info()
	return {
		bounding_radius=200,
		materials={'mat1','windows'}
	}
end
function boringHighRise_dynamic(lod)
	use_material("mat1", 'windows')
	tri(v(-20,0,0),v(20,0,0),v(-20,20,0))
end
function boringHighRise_static(lod)
	set_material("mat1", 0.5,0.5,0.5,1)
	use_material("mat1")
	extrusion(v(0,0,20), v(0,0,-20), v(0,1,0), 1.0,
			v(-20,0,0), v(20,0,0), v(20,200,0), v(-20,200,0))
	set_material("windows", 0,0,0,1, 1,1,1,50, .5,.5,0)
	use_material("windows")
	zbias(1, v(0,0,20), v(0,0,1))
	for y = 4,198,5 do
		for x = -17,16,4 do
			quad(v(x,y,20), v(x+2,y,20), v(x+2,y+3,20), v(x,y+3,20))
		end
	end
	zbias(1, v(-20,0,0), v(-1,0,0))
	for y = 4,198,5 do
		for x = -17,16,4 do
			quad(v(-20,y,x), v(-20,y,x+2), v(-20,y+3,x+2), v(-20,y+3,x))
		end
	end
	zbias(1, v(20,0,0), v(1,0,0))
	for y = 4,198,5 do
		for x = -17,16,4 do
			quad(v(20,y+3,x), v(20,y+3,x+2), v(20,y,x+2), v(20,y,x))
		end
	end
	zbias(1, v(0,0,-20), v(0,0,-1))
	for y = 4,198,5 do
		for x = -15,17,4 do
			quad(v(-x,y+3,-20), v(-x+2,y+3,-20), v(-x+2,y,-20), v(-x,y,-20))
		end
	end
	zbias(0)
	
end

function biodome(lod, trans)
	local d = 1/math.sqrt(2)
	local height = 40
	use_material('base')
	local yank=-690
	xref_bezier_4x4(32, 1, trans*v(0,0,500), trans*v(0,0.25*height,500), trans*v(0,0.75*height,500), trans*v(0,height,500),
			trans*v(yank,0,500), trans*v(yank,0.25*height,500), trans*v(yank,0.75*height,500), trans*v(yank,height,500),
			trans*v(yank,0,-500), trans*v(yank,0.25*height,-500), trans*v(yank,0.75*height,-500), trans*v(yank,height,-500),
			trans*v(0,0,-500), trans*v(0,0.25*height,-500), trans*v(0,0.75*height,-500), trans*v(0,height,-500))
	use_material('green')
	zbias(1, v(0,height,0), v(0,1,0))
	local s = 660
	bezier_3x3(16, 16, trans*v(d*500,height,d*-500), trans*v(0,height,-s), trans*v(d*-500,height,d*-500),
		trans*v(s,height,0), trans*v(0,height,0), trans*v(-s,height,0),
		trans*v(d*500,height,d*500), trans*v(0,height,s), trans*v(d*-500,height,d*500))
	use_material('dome')
	bezier_3x3(16, 16, trans*v(d*500,height,d*-500), trans*v(0,height,-s), trans*v(d*-500,height,d*-500),
		trans*v(s,height,0), trans*v(0,500,0), trans*v(-s,height,0),
		trans*v(d*500,height,d*500), trans*v(0,height,s), trans*v(d*-500,height,d*500))
	zbias(0)
end
function biodomes_info()
	return {
		bounding_radius=1000,
		materials={'base','green','dome'}
	}
end
function biodomes_static(lod)
	set_material('base', .4,.4,.5,1, .5,.5,.7,40)
	set_material('green', .1,.6,.1,1)
	set_material('dome', .5,.5,1,.3, 1,1,1,100)
	m = Mat4x4.translate(v(0,0,0))
	biodome(lod, m)
--	m = Mat4x4.scale(v(0.5,0.5,0.5)) * Mat4x4.translate(v(600,0,0))
--	biodome(lod, m)

	use_material("base")
end
	
function test_info()
	return { lod_pixels={30,60,100,0},
		bounding_radius = 10.0,
		tags = {'building', 'turd'},
		materials = {'red', 'shinyred'}
	}
end
function test_static(lod)
	set_material("red", 1,0,0,1)
	set_material("shinyred", 1,0,0,0.5, 1,1,1,50)
	use_material("red")
	xref_flat(16, v(0,0,1),
		{v(4,0,0)}, -- straight line bit
		{v(4.5,-0.5,0),v(5,0,0)}, -- quadric bezier bit
		{v(5,0.5,0),v(4,0,0),v(4,1,0)}, -- cubic bezier bit
		{v(3,0.5,0)}, -- straight line bit
		{v(3,0.3,0)} -- etc
		)
	zbias(1, v(0,5,0), v(0,0,1))
	geomflag(0x8000)
	text("LOD: " .. tostring(lod), v(0,5,0), v(0,0,1), v(1,1,0):norm(), 1.0)
	geomflag(0)
	zbias(0)
	use_material("red")
	xref_cylinder(lod*4, v(5,0,0), v(10,0,0), v(0,1,0), 1.0)
	use_material("shinyred")
	xref_circle(9, v(4,5,0), v(0,0,1), v(1,0,0), 1.0)
	tri(v(12,3,0),v(13,3,0), v(12,4,0))
	xref_tri(v(13,3,0),v(14,3,0), v(13,4,0))
	xref_quad(v(6,6,0), v(7,6,0), v(7,7,0),v(6,7,0))
--[[	
	geomflag(0x8000)
	xref_bezier_3x3(16, 16,
			v(0,0,0), v(1,-1,0), v(2,0,0),
			v(-1,1,0), v(1,1,8), v(3,1,0),
			v(0,2,0), v(1,3,0), v(2,2,0))
			--]]
--[[	
	xref_bezier_4x4(32, 32,
			v(0,0,0), v(1,0,0), v(2,0,0), v(3,0,0),
			v(0,1,0), v(1,1,5), v(2,1,0), v(3,1,0),
			v(0,2,0), v(1,2,0), v(2,2,0), v(3,2,0),
			v(0,4,0), v(1,4,0), v(1,4,0), v(1,3,0))
			--]]
--[[
	extrusion(v(0,0,0), v(0,0,-5), v(0,1,0), 1.0,
		v(1,0,0), v(0.5, 0.8, 0), v(0,1,0), v(-0.5,0.8,0), v(-1,0,0),
		v(0,-0.5,0))
		--]]
end
poo = 0
function test_dynamic(lod)
	poo = poo + 0.005
	set_material("red", math.sin(poo)*math.sin(poo), 0.5, 0.5, 1)
	use_material("red")
	xref_cylinder(16, v(-8,0,0), v(-8,5,0), v(1,0,0), math.abs(math.sin(poo)))
	circle(9, v(5*math.sin(poo),5*math.cos(poo),0), v(0,0,1), v(1,0,0), 1.0)

	local ang = 2*math.pi*get_arg(0)
	call_model("blob", v(0,0,-20), v(1,0,0),
	v(0,math.sin(ang),math.cos(ang)),1.0)
end

function blob_info()
	return {
		bounding_radius=8,
		materials={'blue'}
	}
end
function blob_static(lod)
	set_material("blue", 0,0,1,1)
	use_material("blue")
	cylinder(16, v(-5,0,0), v(-5,5,0), v(1,0,0), 1.0)
	text("blob_static()", v(-5,-2,0), v(0,0,1), v(1,0,0), 0.5)
	xref_thruster(v(5,0,0), v(0,0,-1), 10)
	xref_thruster(v(5,0,0), v(0,0,1), 10)
	xref_thruster(v(5,0,0), v(0,1,0), 5)
	xref_thruster(v(5,0,0), v(0,-1,0), 5)
	thruster(v(5,0,-5), v(1,0,0), 5, true)
	thruster(v(5,0,5), v(1,0,0), 5, true)
	thruster(v(-5,0,-5), v(-1,0,0), 5, true)
	thruster(v(-5,0,5), v(-1,0,0), 5, true)
end

m = Mat4x4.rotate(math.pi*0.25,v(1,1,1))
m:print()
m = m:inverse()
m:print()
a = (m*v(1,0,0))
a:print()

register_models("blob","test", "towerOfShit",
"boringHighRise","clockhand","clock","church", "skyscraper1", "building1",
"building2",'biodomes','nosewheel', 'nosewheelunit', 'mainwheel',
'mainwheelunit', 'ladybird')
