-- Standalone Lua contract double: checks importer geometry and controller semantics.
AxisPosition = {new = function(x,y,z) return {x=x,y=y,z=z} end}
ArcPlane = {XY='XY'}
ToolpathUnits = {Millimeters='mm'}
Toolpath = {new = function()
    local p = {commands={}, metadata={}}
    function p:set_metadata(k,v) self.metadata[k]=v end
    function p:set_current_position(v) self.current=v end
    function p:record(kind,finish,feed,line,text,center,cw)
        self.commands[#self.commands+1]={kind=kind,start=self.current,finish=finish,feed=feed,line=line,text=text,center=center,cw=cw}
        self.current=finish
    end
    function p:add_tool_change(tool,line,text) self:record('tool',self.current,0,line,text); self.commands[#self.commands].tool=tool end
    function p:add_spindle(speed,line,text) self:record('spindle',self.current,0,line,text); self.commands[#self.commands].speed=speed end
    function p:add_rapid(v,line,text) self:record('rapid',v,0,line,text) end
    function p:add_linear(v,feed,line,text) self:record('linear',v,feed,line,text) end
    function p:add_arc(v,center,cw,plane,feed,line,text) self:record('arc',v,feed,line,text,center,cw) end
    return p
end}
dofile('bin/data/machines/generic_3x_pcni/import.lua')
local function near(a,b) assert(math.abs(a-b)<1e-6, tostring(a)..' ~= '..tostring(b)) end
local prefix = '[PARAMETRI01]\nN10 LX=500 LY=300 LZ=18 Z=PRK\n%\n[CONTORNATURA01]\n'
local p=convert_nc('[COMMENTO]\nX999 Y999\n'..prefix..[[
N20 XO0 YO0 G0
N30 PAN=1 ST1="X123 Y456" L=PATC
N40 TP=1 X10 Y280 G40 PRF=-10 QVF=2 L=PON
N50 G1 ZI15 F3
N60 X20 Y270 ZI-2
N70 G4 X30 Y260 R10 F1
N80 G5 X40 Y250 R-10
N90 QUP=12 L=PUP
N100 TP=1 X50 Y240 G40 PRF=-8 QVF=2 L=PON
N110 G1 ZI10
N120 G0 ZI-4 Z999
%
[FORATURA01]
N130 X999 Y999 PRF=10 L=PFOR
]])
assert(p.machine_name=='generic_3x_pcni' and #p.commands==8)
assert(p.metadata.LX=='500' and p.metadata.LY=='300' and p.metadata.LZ=='18')
local c=p.commands
near(c[1].start.x,10); near(c[1].start.y,20); near(c[1].start.z,10)
near(c[1].finish.z,-5); near(c[1].feed,3000); assert(c[1].line==50)
near(c[2].finish.x,20); near(c[2].finish.y,30); near(c[2].finish.z,-3)
assert(c[3].cw==false); near(c[3].center.x,20); near(c[3].center.y,40)
assert(c[4].cw==true); near(c[4].center.x,40); near(c[4].center.y,40); near(c[4].feed,1000)
near(c[5].finish.z,12); assert(c[5].kind=='rapid')
near(c[6].finish.z,8); near(c[6].finish.y,60)
near(c[7].finish.z,-2); near(c[7].feed,2000)
near(c[8].finish.z,2); assert(c[8].kind=='rapid')
local function rejects(body,expected)
    local ok,message=pcall(convert_nc,body)
    assert(not ok and message:find(expected,1,true),tostring(message))
end
rejects('[CONTORNATURA01]\nX10 Y20 PRF=-5 L=PON','LY gerekli')
rejects(prefix..'G1 X10','L=PON gerekli')
rejects(prefix..'X0 Y300 PRF=0 L=PON\nG4 X50 Y250 R1','gecersiz R')
rejects(prefix..'X0 Y300 PRF=0 L=PON\nG5 X10 Y290 R10','negatif R')
rejects(prefix..'X0 Y300 PRF=0 L=PON\nG4 X10 Y290 I1 J1','tangent modu')
rejects(prefix..'X10 Y20 PRF=5 L=PFOR','desteklenmeyen hareket makrosu')
rejects(prefix..'X0 Y300 PRF=0 L=PON\nL=PUP','QUP gerekli')
local q=convert_nc(prefix..'X10Y280PRF=-5QVF=2L=PON\nG1X20Y270ZI6F2 ; X999\n')
assert(#q.commands==1); near(q.commands[1].finish.x,20); near(q.commands[1].finish.z,-1)
assert(#convert_nc(prefix..'X10 Y280 PRF=-5 L=PON').commands==0)
print('PCNI importer checks passed (coordinates, arcs, macros, feed, sections, errors).')

local drill_prefix = '[PARAMETRI01]\nLX=1000 LY=600 LZ=18\n%\n[CONTORNATURA01]\n'
local drill_line = 'X19.000 Y507.000 PRF=14.000 T=7 QVF=3 QVR=4500 PNEXT=1 L=G99'
local d = convert_nc(drill_prefix .. 'QUP=10 L=PUP\nN70 ' .. drill_line .. '\nN80 X29 Y497 PRF=8 T=7 QVF=2 QVR=4500 PNEXT=0 L=G99\n')
assert(#d.commands==8)
local dc=d.commands
assert(dc[1].kind=='tool' and dc[1].tool==7)
assert(dc[2].kind=='spindle' and dc[2].speed==4500)
assert(dc[3].kind=='rapid'); near(dc[3].finish.x,19); near(dc[3].finish.y,93); near(dc[3].finish.z,10)
assert(dc[4].kind=='linear'); near(dc[4].finish.z,-14); near(dc[4].feed,3000)
assert(dc[4].line==70 and dc[4].text:find(drill_line,1,true))
near(dc[5].finish.z,10)
assert(dc[6].kind=='rapid'); near(dc[6].finish.x,29); near(dc[6].finish.y,103); near(dc[6].finish.z,10)
near(dc[7].finish.z,-8); near(dc[7].feed,2000); near(dc[8].finish.z,10)
rejects(drill_prefix .. drill_line,'G99 icin grup sonunda')
rejects(drill_prefix .. 'QUP=10 L=PUP\nX19 Y507 PRF=14 T=7 QVR=4500 L=G99','QVF gerekli')
local changed=convert_nc(drill_prefix .. 'QUP=10 L=PUP\n'..drill_line..'\nQUP=20 L=PUP\nX29 Y497 PRF=8 T=8 QVF=2 QVR=5000 L=G99')
near(changed.commands[#changed.commands].finish.z,20)
local tools,speeds=0,0
for _, command in ipairs(changed.commands) do
    if command.kind=='tool' then tools=tools+1; if tools==2 then assert(command.tool==8) end end
    if command.kind=='spindle' then speeds=speeds+1; if speeds==2 then assert(command.speed==5000) end end
end
assert(tools==2 and speeds==2)
print('G99 drilling checks passed (LY, depth, QUP, feed, tool, spindle, consecutive holes).')
local first_group=convert_nc(drill_prefix..drill_line..'\nX29 Y497 PRF=8 T=7 QVF=2 QVR=4500 L=G99\nQUP=12 L=POFF\n')
near(first_group.commands[3].finish.z,12)
near(first_group.commands[3].finish.y,93)
near(first_group.commands[4].finish.z,-14)
near(first_group.commands[#first_group.commands].finish.z,12)
local override=convert_nc(drill_prefix..'QUP=10 L=PUP\n'..drill_line..'\nQUP=15 L=POFF\n')
near(override.commands[#override.commands].finish.z,15)
rejects(drill_prefix..drill_line..'\n%\n[FORATURA01]\nQUP=99 L=POFF','G99 icin grup sonunda')
rejects(drill_prefix..drill_line..'\nX0 Y0 PRF=-10 L=PON\nQUP=99 L=PUP','G99 icin grup sonunda')
print('Initial G99 group clearance checks passed.')
if arg and arg[1] then
    local file=assert(io.open(arg[1],'rb'))
    local source=file:read('*a'); file:close()
    local real=convert_nc(source)
    assert(#real.commands>0)
    local holes=0
    for _, command in ipairs(real.commands) do
        if command.kind=='linear' and command.text:find('L=G99',1,true) then
            holes=holes+1
            near(command.start.z,10); near(command.finish.z,-14)
            if holes==1 then near(command.finish.x,19); near(command.finish.y,1593) end
        end
    end
    assert(holes==79, 'Unexpected hole count: '..holes)
    print('Actual PCNI converted: '..#real.commands..' commands, '..holes..' holes; first Y=1593, Z=-14, clearance=10.')
end