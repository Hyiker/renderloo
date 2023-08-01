
add_rules("mode.debug", "mode.release")


-- glslc toolchain
-- reference: https://github.com/xmake-io/xmake/blob/63e4d08d20a935acf47ac86beb587e9261b78506/xmake/rules/utils/glsl2spv/xmake.lua
rule("glsl2hpp")
    set_extensions(".vert", ".tesc", ".tese", ".geom", ".comp", ".frag", ".compute", ".mesh",
     ".task", ".rgen", ".rint", ".rahit", ".rchit", ".rmiss", ".rcall", ".glsl")
    before_buildcmd_file(function (target, batchcmds, sourcefile_glsl, opt)
        import("lib.detect.find_tool")
        import("lib.detect.find_program")

        -- get glsl
        local glslc
        glslc = find_tool("glslc")
        assert(glslc, "glslc not found!")

        -- glsl to spv
        local outputdir = target:extraconf("rules", "glsl2hpp", "outputdir")
        local defines = target:extraconf("rules", "glsl2hpp", "defines")
        assert(outputdir, "outputdir not set!")
        local spvfilepath = path.join(outputdir, path.filename(sourcefile_glsl) .. ".spv")
        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.glsl2hpp %s", sourcefile_glsl)
        batchcmds:mkdir(outputdir)
        local argv = {"-fauto-map-locations", "-fauto-bind-uniforms", "-fauto-combined-image-sampler",
         "--target-env=opengl", "-o", path(spvfilepath), path(sourcefile_glsl)}
        if defines then
            for _, define in ipairs(defines) do
                table.insert(argv, "-D" .. define)
            end
        end
        batchcmds:vrunv(glslc.program, argv)

        -- do bin2c
        local outputfile = spvfilepath:gsub(".spv$", "") .. ".hpp"
        -- get header file
        local headerdir = outputdir
        local headerfile = path.join(headerdir, path.filename(outputfile))
        target:add("includedirs", headerdir)
    
        -- add commands
        local argv = {"r", "spv2hpp", path.absolute(spvfilepath), path.absolute(headerfile)}
        batchcmds:vrunv(os.programfile(), argv, {envs = {XMAKE_SKIP_HISTORY = "y"}})

        os.rm(path.absolute(spvfilepath))
        -- add deps
        batchcmds:add_depfiles(sourcefile_glsl)
        batchcmds:set_depmtime(os.mtime(outputfile))
        batchcmds:set_depcache(target:dependfile(outputfile))
    end)
    on_clean(function (target)
        local outputdir = target:extraconf("rules", "glsl2hpp", "outputdir")
        if outputdir then
            os.rm(outputdir)
        end
    end)
rule_end()

target("spv2hpp")
    set_kind("binary")
    add_files("spv2hpp.cpp")
    set_languages("cxx17")
    add_defines("_CRT_SECURE_NO_WARNINGS")

    set_policy("build.warning", true)
    set_warnings("allextra")

    set_policy('build.across_targets_in_parallel', false)

    after_build(function (target)
        -- os.cp("spv2hpp.exe", path.join("$(environment)", "bin"))
        -- import("package.tools.xmake").install(package, {plat = os.host(), arch = os.arch()})
    end)
