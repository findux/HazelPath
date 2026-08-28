-- Generic 3-axis reverse post-processor example.
-- Contract: convert_nc(nc_text) must return a Toolpath object.
-- I/J/K arc centers are offsets from the arc start position.
-- R arcs select the center that produces the requested G2/G3 direction.

local function copy_position(position)
    local result = AxisPosition.new(position.x, position.y, position.z)
    result.a = position.a
    result.b = position.b
    result.c = position.c
    return result
end

local function approximately(value, expected)
    return math.abs(value - expected) < 0.000001
end

local function plane_coordinates(position, plane)
    if plane == ArcPlane.XY then
        return position.x, position.y
    elseif plane == ArcPlane.XZ then
        return position.z, position.x
    end
    return position.y, position.z
end

local function position_with_plane_coordinates(position, plane, u, v)
    local result = copy_position(position)
    if plane == ArcPlane.XY then
        result.x = u
        result.y = v
    elseif plane == ArcPlane.XZ then
        result.z = u
        result.x = v
    else
        result.y = u
        result.z = v
    end
    return result
end


local function arc_center_from_radius(start_position, end_position, radius, clockwise, plane, source_line)
    local start_u, start_v = plane_coordinates(start_position, plane)
    local end_u, end_v = plane_coordinates(end_position, plane)
    local delta_u = end_u - start_u
    local delta_v = end_v - start_v
    local chord = math.sqrt(delta_u * delta_u + delta_v * delta_v)
    local tolerance = 0.000001

    if radius <= tolerance then
        error("G2/G3 R pozitif olmali; satir " .. source_line)
    end
    if chord <= tolerance then
        error("G2/G3 tam daire R ile tanimlanamaz; I/J/K kullanin; satir " .. source_line)
    end
    if chord > 2.0 * radius + tolerance then
        error("G2/G3 R yaricapi baslangic ve bitis noktalarina yetmiyor; satir " .. source_line)
    end

    local middle_u = (start_u + end_u) * 0.5
    local middle_v = (start_v + end_v) * 0.5
    local half_chord = chord * 0.5
    local height_squared = radius * radius - half_chord * half_chord
    local height = math.sqrt(math.max(0.0, height_squared))
    local perpendicular_u = -delta_v / chord
    local perpendicular_v = delta_u / chord

    local center1_u = middle_u + perpendicular_u * height
    local center1_v = middle_v + perpendicular_v * height
    local center2_u = middle_u - perpendicular_u * height
    local center2_v = middle_v - perpendicular_v * height
    local start1_u = start_u - center1_u
    local start1_v = start_v - center1_v
    local end1_u = end_u - center1_u
    local end1_v = end_v - center1_v
    local cross1 = start1_u * end1_v - start1_v * end1_u
    local center1_matches_direction =
        (clockwise and cross1 <= tolerance) or
        (not clockwise and cross1 >= -tolerance)

    if center1_matches_direction then
        return position_with_plane_coordinates(start_position, plane, center1_u, center1_v)
    end
    return position_with_plane_coordinates(start_position, plane, center2_u, center2_v)
end

function convert_nc(nc_text)
    local program = gpr.parse_with_source(nc_text)
    local path = Toolpath.new()
    path.machine_name = "generic_3axis"
    path.source_name = "editor"
    path.units = ToolpathUnits.Millimeters

    local current = AxisPosition.new(0.0, 0.0, 0.0)
    local absolute_mode = true
    local motion_mode = 0
    local arc_plane = ArcPlane.XY
    local feed_rate = 0.0
    path:set_current_position(current)

    for block_index = 0, program:num_blocks() - 1 do
        if of_progress then of_progress(block_index + 1, program:num_blocks()) end
        local block = program:get_block(block_index)
        if not block:is_deleted() then
            local next_position = copy_position(current)
            local center_offset = glm.vec3(0.0, 0.0, 0.0)
            local arc_radius = 0.0
            local has_axis = false
            local has_arc_center = false
            local has_arc_radius = false
            local source_line = block:line_number()

            for chunk_index = 0, block:size() - 1 do
                local chunk = block:get_chunk(chunk_index)
                if chunk:type() == GprChunkType.WordAddress then
                    local word = string.upper(chunk:word())
                    local value = chunk:address():number()

                    if word == "G" then
                        if approximately(value, 0) then motion_mode = 0 end
                        if approximately(value, 1) then motion_mode = 1 end
                        if approximately(value, 2) then motion_mode = 2 end
                        if approximately(value, 3) then motion_mode = 3 end
                        if approximately(value, 17) then arc_plane = ArcPlane.XY end
                        if approximately(value, 18) then arc_plane = ArcPlane.XZ end
                        if approximately(value, 19) then arc_plane = ArcPlane.YZ end
                        if approximately(value, 20) then
                            path.units = ToolpathUnits.Inches
                        end
                        if approximately(value, 21) then
                            path.units = ToolpathUnits.Millimeters
                        end
                        if approximately(value, 90) then absolute_mode = true end
                        if approximately(value, 91) then absolute_mode = false end
                    elseif word == "X" then
                        next_position.x = absolute_mode and value or (current.x + value)
                        has_axis = true
                    elseif word == "Y" then
                        next_position.y = absolute_mode and value or (current.y + value)
                        has_axis = true
                    elseif word == "Z" then
                        next_position.z = absolute_mode and value or (current.z + value)
                        has_axis = true
                    elseif word == "A" then
                        next_position.a = absolute_mode and value or (current.a + value)
                        has_axis = true
                    elseif word == "B" then
                        next_position.b = absolute_mode and value or (current.b + value)
                        has_axis = true
                    elseif word == "C" then
                        next_position.c = absolute_mode and value or (current.c + value)
                        has_axis = true
                    elseif word == "I" then
                        center_offset.x = value
                        has_arc_center = true
                    elseif word == "J" then
                        center_offset.y = value
                        has_arc_center = true
                    elseif word == "K" then
                        center_offset.z = value
                        has_arc_center = true
                    elseif word == "R" then
                        arc_radius = value
                        has_arc_radius = true
                    elseif word == "F" then
                        feed_rate = value
                    end
                end
            end

            local has_motion = has_axis or
                ((motion_mode == 2 or motion_mode == 3) and
                    (has_arc_center or has_arc_radius))
            if has_motion then
                if motion_mode == 0 then
                    path:add_rapid(next_position, source_line, block:source())
                elseif motion_mode == 1 then
                    path:add_linear(next_position, feed_rate, source_line, block:source())
                elseif motion_mode == 2 or motion_mode == 3 then
                    local clockwise = motion_mode == 2
                    local center
                    if has_arc_center then
                        local start_xyz = glm.vec3(current.x, current.y, current.z)
                        local center_xyz = start_xyz + center_offset
                        center = AxisPosition.new(center_xyz.x, center_xyz.y, center_xyz.z)
                    elseif has_arc_radius then
                        center = arc_center_from_radius(
                            current,
                            next_position,
                            arc_radius,
                            clockwise,
                            arc_plane,
                            source_line)
                    else
                        error("G2/G3 I/J/K veya R gerektiriyor; satir " .. source_line)
                    end
                    path:add_arc(
                        next_position,
                        center,
                        clockwise,
                        arc_plane,
                        feed_rate,
                        source_line,
                        block:source())
                end
                current = next_position
            end
        end
    end

    path:set_metadata("converter", "generic_3axis/import.lua")
    path:set_metadata("contract_version", "1")
    return path
end