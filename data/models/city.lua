
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
	
register_models("towerOfShit",
"boringHighRise","clockhand","clock","church", "skyscraper1", "building1",
"building2",'biodomes')
