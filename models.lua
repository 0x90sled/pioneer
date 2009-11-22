function test_static()
	dec_material("red")
	dec_material("orange")
	set_material("red", 1,0,0)
	set_material("orange", 1,0.5,0)
	ring(32, v(5,0,0), v(5,5,0), v(1,0,0), 1.0)
	circle(9, v(0,5,0), v(0,0,1), v(1,0,0), 1.0)
	tri(v(2,0,0),v(3,0,0), v(2,1,0))
	use_material("orange")
	tri(v(2,3,0),v(3,3,0), v(2,4,0))
	tri(v(3,3,0),v(4,3,0), v(3,4,0))
	quad(v(6,6,0), v(7,6,0), v(7,7,0),v(6,7,0))
	use_material("red")
	zbias(1, v(0,5,0), v(0,0,1))
	text("ABCDEfghi Hello 1234", v(0,5,0), v(0,0,1), v(1,1,0):norm(), 1.0)
	zbias(0)
	callmodel("blob")
end
poo = 0
function test_dynamic()
	poo = poo + 0.005
	set_material("red", math.sin(poo)*math.sin(poo), 0.5, 0.5)
	use_material("red")
	cylinder(16, v(-8,0,0), v(-8,5,0), v(1,0,0), math.abs(math.sin(poo)))
	circle(9, v(5*math.sin(poo),5*math.cos(poo),0), v(0,0,1), v(1,0,0), 1.0)
end

function blob_static()
	dec_material("blue")
	set_material("blue", 0,0,1)
	use_material("blue")
	cylinder(16, v(-5,0,0), v(-5,5,0), v(1,0,0), 1.0)
	text("blob_static()", v(-5,-2,0), v(0,0,1), v(1,0,0), 0.5)
end
function blob_dynamic()
end
	

x = v(1,2,3,5)+v(2,3,4)
x = x-v(3,1,1)
x:print()
y = v(math.sin(10),math.cos(10),0):cross(v(0,1,0))
y:print()

z = 3.1*v(1,2,3)
z:print()

z = v(1,2,3)/10
z = z:norm()
z:print()
a = v(z:len(), z:dot(y), z:len())
a:print()
print(v(1,0,0):dot(v(0.5,1,0):norm()))

function hi(x)
	x = x + 1
	return function(val)
		print(tostring(x) .. " sod you " .. tostring(val))
	end
end

x = hi(1)
x("my friend")
-- comment

register_models("blob","test")
