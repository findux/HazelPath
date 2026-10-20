-- ADekoCAM PCNI reverse post. Coordinates returned in CAM space (surface Z=0).
-- Deliberately parse PCNI words directly: the GPR lexer has single-letter words.
local function fail(line, message)
    error("PCNI satir " .. line .. ": " .. message)
end

local function words(text)
    local result = {}
    -- Strip quoted tool names before looking for words or comments.
    text = text:gsub('"[^"]*"', '""'):gsub(';.*$', ''):gsub('%b()', ''):upper()
    local cursor = 1
    while cursor <= #text do
        local first, last, key = text:find('([A-Z_]+)', cursor)
        if not first then break end
        local tail = text:sub(last + 1)
        local prefix, value = tail:match('^(%s*=?%s*)([+-]?%d*%.?%d+)')
        if value then
            result[key] = tonumber(value)
            cursor = last + #prefix + #value + 1
        else
            local assignment, symbol = tail:match('^(%s*=%s*)([A-Z_][A-Z_0-9%.]*)')
            if symbol then
                result[key] = symbol
                cursor = last + #assignment + #symbol + 1
            else
                cursor = last + 1
            end
        end
    end
    return result
end

-- A program may start with drilling; its closing PUP/POFF supplies clearance.
local function following_group_qup(lines, start_index)
    for index = start_index, #lines do
        local source = lines[index]
        local clean = source:gsub(';.*$', ''):upper()
        if clean:match('^%s*%%') or clean:match('^%s*%[') then break end
        if not clean:match('^%s*/') then
            local w = words(source)
            if w.L == 'PON' then break end
            if w.L == 'PUP' or w.L == 'POFF' then
                if type(w.QUP) == 'number' and w.QUP >= 0 then return w.QUP end
                break
            end
        end
    end
    return nil
end

local function radius_center(start, finish, radius, clockwise, line)
    radius = math.abs(radius) -- PCNI R sign encodes direction, not a major arc.
    local dx, dy = finish.x - start.x, finish.y - start.y
    local chord = math.sqrt(dx * dx + dy * dy)
    if radius <= 0 or chord < 1e-6 or chord > 2 * radius + 0.002 then
        fail(line, 'G4/G5 gecersiz R yayi')
    end
    local height = math.sqrt(math.max(0, radius * radius - chord * chord / 4))
    local side = clockwise and -1 or 1
    return AxisPosition.new((start.x + finish.x) / 2 - side * dy / chord * height,
        (start.y + finish.y) / 2 + side * dx / chord * height, start.z)
end

function convert_nc(nc_text)
    local path = Toolpath.new()
    path.machine_name = 'generic_3x_pcni'
    path.source_name = 'editor'
    path.units = ToolpathUnits.Millimeters
    path:set_metadata('converter', 'generic_3x_pcni/import.lua')
    path:set_metadata('contract_version', '1')
    local current = AxisPosition.new(0, 0, 0)
    path:set_current_position(current)
    local dimensions, section = {}, nil
    local active, initialized, found_contour = false, false, false
    local mode, feed, line_number = 0, 0, 0
    local last_qup, drill_tool, drill_spindle = nil, nil, nil
    local drill_group_qup = nil
    nc_text = nc_text:gsub('\r\n', '\n'):gsub('\r', '\n')
    local _, newline_count = nc_text:gsub('\n', '')
    local lines = {}
    for source in (nc_text .. '\n'):gmatch('(.-)\n') do lines[#lines + 1] = source end
    for index, source in ipairs(lines) do
        line_number = index
        if of_progress then of_progress(line_number, newline_count + 1) end
        local clean = source:gsub(';.*$', ''):upper()
        local heading = clean:match('^%s*%[([A-Z_0-9]+)%]')
        if heading then
            section = heading
            if section == 'CONTORNATURA01' then found_contour = true end
        elseif clean:match('^%s*%%') then
            section = nil
        elseif not clean:match('^%s*/') then
            local w = words(source)
            local line = w.N or line_number
            if section == 'PARAMETRI01' then
                for _, key in ipairs({'LX', 'LY', 'LZ'}) do
                    if w[key] ~= nil then
                        if type(w[key]) ~= 'number' or w[key] <= 0 then fail(line, key .. ' pozitif olmali') end
                        dimensions[key] = w[key]
                        path:set_metadata(key, tostring(w[key]))
                    end
                end
            elseif section == 'CONTORNATURA01' then
                local pon, pup = w.L == 'PON', w.L == 'PUP'
                if w.F and w.L ~= 'G99' then feed = w.F * 1000 end
                local has_axis = w.X ~= nil or w.Y ~= nil or w.ZI ~= nil
                -- XO/YO and ordinary Z (including Z=PRK) are not motion axes.
                if w.L == 'G99' then
                    if not dimensions.LY then fail(line, 'Y donusumu icin LY gerekli') end
                    for _, key in ipairs({'X', 'Y', 'PRF', 'T', 'QVF', 'QVR'}) do
                        if type(w[key]) ~= 'number' then fail(line, 'G99 icin ' .. key .. ' gerekli') end
                    end
                    if w.PRF < 0 or w.QVF <= 0 or w.QVR < 0 or w.T < 0 or w.T % 1 ~= 0 then
                        fail(line, 'G99 PRF/T/QVF/QVR degerleri gecersiz')
                    end
                    if drill_group_qup == nil then
                        drill_group_qup = following_group_qup(lines, line_number + 1) or last_qup
                    end
                    if drill_group_qup == nil then
                        fail(line, 'G99 icin grup sonunda veya onceki PUP satirinda QUP gerekli')
                    end
                    -- Each cycle approaches and retracts at the group clearance height.
                    local approach = AxisPosition.new(w.X, dimensions.LY - w.Y, drill_group_qup)
                    if not initialized then
                        path:set_current_position(approach)
                        initialized = true
                    elseif math.abs(current.z - drill_group_qup) > 1e-6 then
                        path:add_rapid(AxisPosition.new(current.x, current.y, drill_group_qup), line, source)
                    end
                    if drill_tool ~= w.T then
                        path:add_tool_change(w.T, line, source)
                        drill_tool = w.T
                    end
                    if drill_spindle ~= w.QVR then
                        path:add_spindle(w.QVR, line, source)
                        drill_spindle = w.QVR
                    end
                    path:add_rapid(approach, line, source)
                    path:add_linear(AxisPosition.new(approach.x, approach.y, -w.PRF), w.QVF * 1000, line, source)
                    path:add_rapid(AxisPosition.new(approach.x, approach.y, drill_group_qup), line, source)
                    current = AxisPosition.new(approach.x, approach.y, drill_group_qup)
                    active, mode = false, 0
                elseif w.L and not pon and not pup then
                    drill_group_qup = nil
                    if has_axis or w.PRF then fail(line, 'desteklenmeyen hareket makrosu L=' .. tostring(w.L)) end
                elseif pon or pup or has_axis or w.R or w.I or w.J then
                    if w.G and w.G ~= 0 and w.G ~= 1 and w.G ~= 4 and w.G ~= 5 and w.G ~= 40 then
                        fail(line, 'desteklenmeyen G komutu: ' .. tostring(w.G))
                    end
                    if not dimensions.LY then fail(line, 'Y donusumu icin LY gerekli') end
                    if pon and (w.X == nil or w.Y == nil or w.PRF == nil) then
                        fail(line, 'PON icin X, Y ve PRF gerekli')
                    end
                    if not pon and not pup and not active then fail(line, 'hareketten once L=PON gerekli') end
                    local next_pos = AxisPosition.new(current.x, current.y, current.z)
                    if w.X then next_pos.x = w.X end
                    if w.Y then next_pos.y = dimensions.LY - w.Y end
                    if pon then
                        next_pos.z = -w.PRF
                        drill_tool, drill_spindle = nil, nil
                        drill_group_qup = nil
                        active, mode, feed = true, 0, (w.QVF or 2) * 1000
                    elseif pup then
                        if type(w.QUP) ~= 'number' or w.QUP < 0 then fail(line, 'PUP icin pozitif QUP gerekli') end
                        next_pos.z = w.QUP
                        last_qup = w.QUP
                        drill_group_qup = nil
                    elseif w.ZI then
                        next_pos.z = current.z - w.ZI
                    end
                    if w.F and w.L ~= 'G99' then feed = w.F * 1000 end -- Post F is m/min; Toolpath is mm/min.
                    if w.G == 0 or w.G == 1 or w.G == 4 or w.G == 5 then mode = w.G end
                    if not initialized then
                        -- The first PON defines the initial point; no invented move from origin.
                        path:set_current_position(next_pos)
                        initialized = true
                    elseif pon or pup or mode == 0 then
                        path:add_rapid(next_pos, line, source)
                    elseif mode == 1 then
                        path:add_linear(next_pos, feed, line, source)
                    elseif mode == 4 or mode == 5 then
                        local clockwise = mode == 5 -- Undo LY reflection: recover original CAM direction.
                        if w.I or w.J then
                            fail(line, 'I/J tangent modu bu postta kapali; R yayi gerekli')
                        end
                        if not w.R then fail(line, 'G4/G5 icin R gerekli') end
                        if (clockwise and w.R >= 0) or (not clockwise and w.R <= 0) then
                            fail(line, 'G4 pozitif R, G5 negatif R gerektiriyor')
                        end
                        local center = radius_center(current, next_pos, w.R, clockwise, line)
                        path:add_arc(next_pos, center, clockwise, ArcPlane.XY, feed, line, source)
                    end
                    current = next_pos
                    if pup then active, mode = false, 0 end
                elseif w.G == 0 or w.G == 1 or w.G == 4 or w.G == 5 then
                    mode = w.G
                end
            end
        end
    end
    if not found_contour then error('PCNI CONTORNATURA01 bolumu bulunamadi') end
    return path
end
