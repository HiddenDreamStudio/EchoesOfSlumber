newoption
{
    trigger = "sdl_backend",
    value = "BACKEND",
    description = "SDL backend to use",
    allowed = {
        { "auto", "Auto-detect best backend" },
        { "opengl", "OpenGL backend" },
        { "vulkan", "Vulkan backend" },
        { "d3d11", "Direct3D 11 backend" },
        { "d3d12", "Direct3D 12 backend" }
    },
    default = "auto"
}

newoption
{
    trigger = "discord_sdk_url",
    value = "URL",
    description = "Override the Discord Social SDK download URL (for CI or custom hosting)"
}

newoption
{
    trigger = "toolset_ver",
    value = "VERSION",
    description = "Override the default toolset version (e.g. v143, v145)"
}

function download_progress(total, current)
    local ratio = current / total
    ratio = math.min(math.max(ratio, 0), 1)
    local percent = math.floor(ratio * 100)
    print("Download progress (" .. percent .. "%/100%)")
end

function check_sdl3()
    os.chdir("external")
    
    -- Get the cached versions (will fetch only once)
    local versions = get_latest_versions()
    local sdl3_version = versions.sdl3
    local sdl3_folder = "SDL3-" .. sdl3_version
    local sdl3_zip = "SDL3-devel-" .. sdl3_version .. "-VC.zip"
    
    if(os.isdir("SDL3") == false) then
        if(not os.isfile(sdl3_zip)) then
            print("SDL3 v" .. sdl3_version .. " not found, downloading from GitHub")
            local download_url = "https://github.com/libsdl-org/SDL/releases/download/release-" .. sdl3_version .. "/SDL3-devel-" .. sdl3_version .. "-VC.zip"
            local result_str, response_code = http.download(download_url, sdl3_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping SDL3 to " .. os.getcwd())
        zip.extract(sdl3_zip, os.getcwd())
        
        -- Rename the extracted folder to simple name
        if os.isdir(sdl3_folder) then
            os.rename(sdl3_folder, "SDL3")
            print("Renamed " .. sdl3_folder .. " to SDL3")
        end
        
        os.remove(sdl3_zip)
    else
        print("SDL3 already exists")
    end
    
    os.chdir("../")
end

-- Global variable to cache versions
local cached_versions = nil

function get_latest_versions()
    -- Return cached versions if already fetched
    if cached_versions then
        return cached_versions
    end
    
    -- Define repositories and their fallback versions
    local repos = {
        sdl3 = {
            repo = "libsdl-org/SDL",
            fallback = "3.2.22"
        },
        box2d = {
            repo = "erincatto/box2d",
            fallback = "3.1.1"
        },
        libpng = {
            repo = "TheUnrealZaka/libpng",
            fallback = "1.6.44"
        },
        libjpeg_turbo = {
            repo = "TheUnrealZaka/libjpeg-turbo",
            fallback = "3.0.4"
        },
        pugixml = {
            repo = "zeux/pugixml",
            fallback = "1.14"
        },
        sdl3_image = {
            repo = "libsdl-org/SDL_image",
            fallback = "3.0.0"
        },
        sdl3_ttf = {
            repo = "libsdl-org/SDL_ttf",
            fallback = "3.2.2"
        },
        tracy = {
            repo = "wolfpld/tracy",
            fallback = "0.11.1"
        }
    }
    
    local versions = {}
    
    print("Fetching latest versions for all externals...")
    
    for name, info in pairs(repos) do
        print("Checking latest " .. name .. " version...")
        local result_str, response_code = http.get("https://api.github.com/repos/" .. info.repo .. "/releases/latest")
        
        -- Check for successful response (200 or "OK")
        if (response_code == 200 or response_code == "OK") and result_str then
            -- Parse JSON to extract tag_name
            local tag_match = string.match(result_str, '"tag_name"%s*:%s*"([^"]+)"')
            if tag_match then
                -- Extract version number from tag, handling prefixes like "v", "release-", etc.
                -- Try multiple patterns in order
                local version = string.match(tag_match, "^release%-(.+)$")  -- "release-X.Y.Z"
                if not version then
                    version = string.match(tag_match, "^v(.+)$")  -- "vX.Y.Z"
                end
                if not version then
                    version = tag_match  -- "X.Y.Z" (no prefix)
                end
                
                versions[name] = version
                print("Latest " .. name .. " version found: " .. version .. " (from tag: " .. tag_match .. ")")
            else
                versions[name] = info.fallback
                print("Could not parse " .. name .. " version from API response, using fallback: " .. info.fallback)
            end
        else
            versions[name] = info.fallback
            print("Could not fetch latest " .. name .. " version from GitHub API (response code: " .. tostring(response_code) .. "), using fallback: " .. info.fallback)
        end
        
        -- Add a small delay between API calls to avoid rate limiting
        if next(repos, name) then  -- Not the last iteration
            print("Waiting briefly to avoid rate limiting...")
            os.execute("ping 127.0.0.1 -n 2 > nul")  -- 1 second delay on Windows
        end
    end    
    -- Cache the results
    cached_versions = versions
    return versions
end

function check_box2d()
    os.chdir("external")
    
    -- Get the cached versions (will fetch only once)
    local versions = get_latest_versions()
    local box2d_version = versions.box2d
    local box2d_folder = "box2d-" .. box2d_version
    local box2d_zip = box2d_folder .. ".zip"
    
    if(os.isdir("box2d") == false) then
        if(not os.isfile(box2d_zip)) then
            print("Box2D v" .. box2d_version .. " not found, downloading from github")
            local download_url = "https://github.com/erincatto/box2d/archive/refs/tags/v" .. box2d_version .. ".zip"
            local result_str, response_code = http.download(download_url, box2d_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " .. os.getcwd())
        zip.extract(box2d_zip, os.getcwd())
        
        -- Rename the extracted folder to simple name
        if os.isdir(box2d_folder) then
            os.rename(box2d_folder, "box2d")
            print("Renamed " .. box2d_folder .. " to box2d")
        end
        
        os.remove(box2d_zip)
    else
        print("Box2D already exists")
    end
    
    os.chdir("../")
end

function check_libjpeg_turbo()
    os.chdir("external")
    
    -- Use fixed version for prebuilt binaries
    local libjpeg_version = "3.1.2"
    local libjpeg_folder = "libjpeg-turbo-" .. libjpeg_version .. "-vc-x64"
    local libjpeg_zip = libjpeg_folder .. ".zip"
    
    if(os.isdir("libjpeg-turbo") == false) then
        if(not os.isfile(libjpeg_zip)) then
            print("libjpeg-turbo v" .. libjpeg_version .. " not found, downloading from GitHub")
            local download_url = "https://github.com/TheUnrealZaka/libjpeg-turbo/releases/download/" .. libjpeg_version .. "/" .. libjpeg_zip
            local result_str, response_code = http.download(download_url, libjpeg_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping libjpeg-turbo to " .. os.getcwd())
        zip.extract(libjpeg_zip, os.getcwd())
        
        -- Rename the extracted folder to simple name
        if os.isdir(libjpeg_folder) then
            os.rename(libjpeg_folder, "libjpeg-turbo")
            print("Renamed " .. libjpeg_folder .. " to libjpeg-turbo")
        end
        
        os.remove(libjpeg_zip)
    else
        print("libjpeg-turbo already exists")
    end
    
    os.chdir("../")
end

function check_pugixml()
    os.chdir("external")
    
    -- Get the cached versions (will fetch only once)
    local versions = get_latest_versions()
    local pugixml_version = versions.pugixml
    local pugixml_folder = "pugixml-" .. pugixml_version
    local pugixml_zip = pugixml_folder .. ".zip"
    
    if(os.isdir("pugixml") == false) then
        if(not os.isfile(pugixml_zip)) then
            print("pugixml v" .. pugixml_version .. " not found, downloading from github")
            local download_url = "https://github.com/zeux/pugixml/archive/refs/tags/v" .. pugixml_version .. ".zip"
            local result_str, response_code = http.download(download_url, pugixml_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping pugixml to " .. os.getcwd())
        zip.extract(pugixml_zip, os.getcwd())
        
        -- Rename the extracted folder to simple name
        if os.isdir(pugixml_folder) then
            os.rename(pugixml_folder, "pugixml")
            print("Renamed " .. pugixml_folder .. " to pugixml")
        end
        
        os.remove(pugixml_zip)
    else
        print("pugixml already exists")
    end
    
    os.chdir("../")
end

function check_sdl3_image()
    os.chdir("external")
    
    -- Use a fixed version for the prebuilt binaries
    local sdl3_image_version = "3.2.4"
    local sdl3_image_folder = "SDL3_image-" .. sdl3_image_version
    local sdl3_image_zip = "SDL3_image-devel-" .. sdl3_image_version .. "-VC.zip"
    
    if(os.isdir("SDL3_image") == false) then
        if(not os.isfile(sdl3_image_zip)) then
            print("SDL3_image v" .. sdl3_image_version .. " not found, downloading prebuilt binaries from GitHub")
            local download_url = "https://github.com/libsdl-org/SDL_image/releases/download/release-" .. sdl3_image_version .. "/SDL3_image-devel-" .. sdl3_image_version .. "-VC.zip"
            local result_str, response_code = http.download(download_url, sdl3_image_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping SDL3_image to " .. os.getcwd())
        zip.extract(sdl3_image_zip, os.getcwd())
        
        -- Rename the extracted folder to simple name
        if os.isdir(sdl3_image_folder) then
            os.rename(sdl3_image_folder, "SDL3_image")
            print("Renamed " .. sdl3_image_folder .. " to SDL3_image")
        end
        
        os.remove(sdl3_image_zip)
    else
        print("SDL3_image already exists")
    end
    
    os.chdir("../")
end

function check_libpng()
    os.chdir("external")
    
    -- Use fixed version for prebuilt binaries
    local libpng_version = "1.2.37"
    local libpng_folder = "libpng-" .. libpng_version .. "-lib"
    local libpng_zip = libpng_folder .. ".zip"
    
    if(os.isdir("libpng") == false) then
        if(not os.isfile(libpng_zip)) then
            print("libpng v" .. libpng_version .. " not found, downloading from GitHub")
            local download_url = "https://github.com/TheUnrealZaka/libpng/releases/download/v" .. libpng_version .. "/" .. libpng_zip
            local result_str, response_code = http.download(download_url, libpng_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        
        -- Create temporary extraction folder
        local temp_folder = "libpng_temp"
        os.mkdir(temp_folder)
        
        print("Unzipping libpng to " .. temp_folder)
        zip.extract(libpng_zip, temp_folder)
        
        -- Check if it extracted to a subfolder or directly
        if os.isdir(temp_folder .. "/" .. libpng_folder) then
            -- Extracted to subfolder, rename it
            os.rename(temp_folder .. "/" .. libpng_folder, "libpng")
            print("Renamed " .. libpng_folder .. " to libpng")
            os.rmdir(temp_folder)
        else
            -- Extracted directly, just rename temp folder
            os.rename(temp_folder, "libpng")
            print("Created libpng directory from extracted files")
        end
        
        os.remove(libpng_zip)
    else
        print("libpng already exists")
    end
    
    os.chdir("../")
end

function check_sdl3_ttf()
    os.chdir("external")
    
    -- Get the cached versions (will fetch only once)
    local versions = get_latest_versions()
    local sdl3_ttf_version = versions.sdl3_ttf
    local sdl3_ttf_folder = "SDL3_ttf-" .. sdl3_ttf_version
    local sdl3_ttf_zip = "SDL3_ttf-devel-" .. sdl3_ttf_version .. "-VC.zip"
    
    if(os.isdir("SDL3_ttf") == false) then
        if(not os.isfile(sdl3_ttf_zip)) then
            print("SDL3_ttf v" .. sdl3_ttf_version .. " not found, downloading prebuilt binaries from GitHub")
            local download_url = "https://github.com/libsdl-org/SDL_ttf/releases/download/release-" .. sdl3_ttf_version .. "/SDL3_ttf-devel-" .. sdl3_ttf_version .. "-VC.zip"
            local result_str, response_code = http.download(download_url, sdl3_ttf_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping SDL3_ttf to " .. os.getcwd())
        zip.extract(sdl3_ttf_zip, os.getcwd())
        
        -- Rename the extracted folder to simple name
        if os.isdir(sdl3_ttf_folder) then
            os.rename(sdl3_ttf_folder, "SDL3_ttf")
            print("Renamed " .. sdl3_ttf_folder .. " to SDL3_ttf")
        end
        
        os.remove(sdl3_ttf_zip)
    else
        print("SDL3_ttf already exists")
    end
    
    os.chdir("../")
end

function check_tracy()
    os.chdir("external")
    
    -- Get the cached versions (will fetch only once)
    local versions = get_latest_versions()
    local tracy_version = versions.tracy
    local tracy_folder = "tracy-" .. tracy_version
    local tracy_zip = tracy_folder .. ".zip"
    
    if(os.isdir("tracy") == false) then
        if(not os.isfile(tracy_zip)) then
            print("Tracy v" .. tracy_version .. " not found, downloading from GitHub")
            local download_url = "https://github.com/wolfpld/tracy/archive/refs/tags/v" .. tracy_version .. ".zip"
            local result_str, response_code = http.download(download_url, tracy_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping Tracy to " .. os.getcwd())
        zip.extract(tracy_zip, os.getcwd())
        
        -- Rename the extracted folder to simple name
        if os.isdir(tracy_folder) then
            os.rename(tracy_folder, "tracy")
            print("Renamed " .. tracy_folder .. " to tracy")
        end
        
        os.remove(tracy_zip)
    else
        print("Tracy already exists")
    end
    
    os.chdir("../")
end

function check_discord_sdk()
    os.chdir("external")
    
    -- Get the latest version from our own repo
    local versions = get_latest_versions()
    -- We assume the SDK version is managed via the repo tags or a default
    local discord_version = "1.9.15332" 
    local discord_folder = "discord_social_sdk"
    local discord_zip = "discord_social_sdk-" .. discord_version .. ".zip"
    
    if(os.isdir(discord_folder) == false) then
        if(not os.isfile(discord_zip)) then
            print("Discord Social SDK v" .. discord_version .. " not found, downloading latest release from GitHub")
            -- We fetch the specific release asset
            local download_url = "https://github.com/" .. repo .. "/releases/download/v" .. discord_version .. "/DiscordSocialSdk-" .. discord_version .. ".zip"
            local result_str, response_code = http.download(download_url, discord_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping Discord Social SDK to " .. os.getcwd())
        zip.extract(discord_zip, os.getcwd())
        
        -- Handle extraction structure
        if os.isdir("discord_social_sdk-" .. discord_version) then
            os.rename("discord_social_sdk-" .. discord_version, discord_folder)
        end
        
        os.remove(discord_zip)
    else
        print("Discord Social SDK already exists")
    end
    
    os.chdir("../")
end

function check_ffmpeg()
    os.chdir("external")
    
    -- FFmpeg shared builds from BtbN (GitHub)
    -- Downloads the latest GPL shared build for win64
    local ffmpeg_zip = "ffmpeg-master-latest-win64-gpl-shared.zip"
    local ffmpeg_extracted = "ffmpeg-master-latest-win64-gpl-shared"
    
    if(os.isdir("ffmpeg") == false) then
        if(not os.isfile(ffmpeg_zip)) then
            print("FFmpeg not found, downloading shared build from BtbN...")
            local download_url = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/" .. ffmpeg_zip
            local result_str, response_code = http.download(download_url, ffmpeg_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
            
            if not os.isfile(ffmpeg_zip) then
                print("")
                print("============================================================")
                print("  FFmpeg DOWNLOAD FAILED")
                print("")
                print("  Could not download from BtbN releases.")
                print("  You can manually download and extract to:")
                print("    build/external/ffmpeg/")
                print("  Expected structure:")
                print("    ffmpeg/include/  (libavcodec, libavformat, etc.)")
                print("    ffmpeg/lib/      (.lib import libraries)")
                print("    ffmpeg/bin/      (.dll shared libraries)")
                print("============================================================")
                print("")
                os.chdir("../")
                return
            end
        end
        
        print("Unzipping FFmpeg to " .. os.getcwd())
        zip.extract(ffmpeg_zip, os.getcwd())
        
        -- Rename the extracted folder to simple name
        if os.isdir(ffmpeg_extracted) then
            os.rename(ffmpeg_extracted, "ffmpeg")
            print("Renamed " .. ffmpeg_extracted .. " to ffmpeg")
        end
        
        os.remove(ffmpeg_zip)
        
        -- Verify extraction succeeded
        if not os.isdir("ffmpeg/include") then
            print("WARNING: FFmpeg extracted but include/ folder not found.")
        else
            print("FFmpeg installed successfully")
        end
    else
        print("FFmpeg already exists")
    end
    
    os.chdir("../")
end

function check_vulkan()
    os.chdir("external")
    
    -- Vulkan SDK components: Headers and Loader
    local vulkan_headers_version = "v1.3.283"
    local vulkan_headers_zip = "Vulkan-Headers.zip"
    
    if(os.isdir("vulkan") == false) then
        if(not os.isfile(vulkan_headers_zip)) then
            print("Vulkan Headers " .. vulkan_headers_version .. " not found, downloading from GitHub")
            local download_url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/" .. vulkan_headers_version .. ".zip"
            local result_str, response_code = http.download(download_url, vulkan_headers_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping Vulkan Headers to " .. os.getcwd())
        zip.extract(vulkan_headers_zip, os.getcwd())
        
        -- Rename the extracted folder to vulkan
        local extracted_folder = "Vulkan-Headers-" .. string.sub(vulkan_headers_version, 2)
        if os.isdir(extracted_folder) then
            os.rename(extracted_folder, "vulkan")
            print("Renamed " .. extracted_folder .. " to vulkan")
        end
        
        os.remove(vulkan_headers_zip)
    else
        print("Vulkan already exists")
    end
    
    os.chdir("../")
end

function build_externals()
    print("Checking external dependencies...")
    
    -- Check if all required dependencies already exist
    local all_exist = true
    if downloadSDL3 and not os.isdir("external/SDL3") then
        all_exist = false
    end
    if downloadLibJPEGTurbo and not os.isdir("external/libjpeg-turbo") then
        all_exist = false
    end
    if downloadPugiXML and not os.isdir("external/pugixml") then
        all_exist = false
    end
    if downloadSDL3Image and not os.isdir("external/SDL3_image") then
        all_exist = false
    end
    if downloadLibPNG and not os.isdir("external/libpng") then
        all_exist = false
    end
    if downloadBox2D and not os.isdir("external/box2d") then
        all_exist = false
    end
    if downloadSDL3TTF and not os.isdir("external/SDL3_ttf") then
        all_exist = false
    end
    if downloadTracy and not os.isdir("external/tracy") then
        all_exist = false
    end
    if downloadDiscordSDK and not os.isdir("external/discord_social_sdk/include") then
        all_exist = false
    end
    if downloadFFmpeg and not os.isdir("external/ffmpeg/include") then
        all_exist = false
    end
    if downloadVulkan and not os.isdir("external/vulkan/include") then
        all_exist = false
    end
    
    -- If all dependencies exist, skip version fetching entirely
    if all_exist then
        print("All external dependencies already installed, skipping checks.")
        return
    end
    
    print("Some dependencies missing, fetching latest versions...")
    
    check_sdl3()
    check_libjpeg_turbo()
    check_pugixml()
    check_sdl3_image()
    if (downloadLibPNG) then
        check_libpng()
    end
    if (downloadBox2D) then
        check_box2d()
    end
    if (downloadSDL3TTF) then
        check_sdl3_ttf()
    end
    if (downloadTracy) then
        check_tracy()
    end
    if (downloadDiscordSDK) then
        check_discord_sdk()
    end
    if (downloadFFmpeg) then
        check_ffmpeg()
    end
    if (downloadVulkan) then
        check_vulkan()
    end
end

function platform_defines()
    filter {"configurations:Debug"}
        defines{"DEBUG", "_DEBUG"}
        symbols "On"

    filter {"configurations:Release"}
        defines{"NDEBUG", "RELEASE"}
        optimize "On"

    filter {"system:windows"}
        defines {"_WIN32", "_WINDOWS"}
        systemversion "latest"

    filter {"system:linux"}
        defines {"_GNU_SOURCE"}
        
    filter{}
end

-- Configuration
downloadSDL3 = true
sdl3_dir = "external/SDL3"

downloadBox2D = true
box2d_dir = "external/box2d"

downloadLibJPEGTurbo = true
libjpeg_turbo_dir = "external/libjpeg-turbo"

downloadPugiXML = true
pugixml_dir = "external/pugixml"

downloadSDL3Image = true
sdl3_image_dir = "external/SDL3_image"

downloadLibPNG = true
libpng_dir = "external/libpng"

downloadSDL3TTF = true
sdl3_ttf_dir = "external/SDL3_ttf"

downloadTracy = true
tracy_dir = "external/tracy"

downloadVulkan = true
vulkan_dir = "external/vulkan"

-- Discord Social SDK: auto-downloads from GitHub Release asset.
-- The SDK zip must be uploaded as a release asset on your repo.
-- To update: download new version from Discord Developer Portal, upload to GitHub Releases.
-- Override URL via: --discord_sdk_url="..." or DISCORD_SDK_URL env var.
downloadDiscordSDK = true
discord_sdk_dir = "external/discord_social_sdk"

-- FFmpeg: auto-downloads shared builds from BtbN GitHub releases
downloadFFmpeg = true
ffmpeg_dir = "external/ffmpeg"

workspaceName = 'PlatformGame'
baseName = path.getbasename(path.getdirectory(os.getcwd()))

-- Use parent folder name for workspace
workspaceName = baseName

if (os.isdir('build_files') == false) then
    os.mkdir('build_files')
end

if (os.isdir('external') == false) then
    os.mkdir('external')
end

workspace (workspaceName)
    location "../"
    configurations { "Debug", "Release" }
    platforms { "x64", "x86" }

    defaultplatform ("x64")

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter { "platforms:x64" }
        architecture "x86_64"

    filter { "platforms:x86" }
        architecture "x86"

    filter {}

    if _OPTIONS["toolset_ver"] then
        toolset (_OPTIONS["toolset_ver"])
    end

    targetdir "bin/%{cfg.buildcfg}/"

if (downloadSDL3 or downloadBox2D or downloadLibJPEGTurbo or downloadPugiXML or downloadSDL3Image or downloadLibPNG or downloadSDL3TTF or downloadTracy or downloadDiscordSDK or downloadVulkan) then
    build_externals()
end

    startproject(workspaceName)

    project (workspaceName)
        kind "ConsoleApp"
        location "build_files/"
        targetdir "../bin/%{cfg.buildcfg}"

        filter "action:vs*"
            debugdir "$(SolutionDir)"

        filter{}

        vpaths 
        {
            ["Header Files/*"] = { "../include/**.h", "../include/**.hpp", "../src/**.h", "../src/**.hpp"},
            ["Source Files/*"] = {"../src/**.c", "../src/**.cpp"},
            ["Windows Resource Files/*"] = {"../src/**.rc", "../src/**.ico"},
        }
        
        files {
            "../src/**.c", 
            "../src/**.cpp", 
            "../src/**.h", 
            "../src/**.hpp", 
            "../include/**.h", 
            "../include/**.hpp"
        }
        
        filter { "system:windows", "action:vs*" }
            files {"../src/*.rc", "../src/*.ico"}

        filter{}

        includedirs { "../src" }
        includedirs { "../include" }
        includedirs { sdl3_dir .. "/include" }
        includedirs { box2d_dir .. "/include" }
        includedirs { libjpeg_turbo_dir .. "/include" }
        includedirs { pugixml_dir .. "/src" }
        includedirs { sdl3_image_dir .. "/include" }
        includedirs { libpng_dir .. "/include" }
        includedirs { sdl3_ttf_dir .. "/include" }
        includedirs { tracy_dir .. "/public" }
        includedirs { ffmpeg_dir .. "/include" }
        includedirs { vulkan_dir .. "/include" }
        -- Discord Social SDK: single-header API (discordpp.h)
        includedirs { discord_sdk_dir .. "/include" }

        cdialect "C17"
        cppdialect "C++17"
        platform_defines()

        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            dependson {"box2d", "pugixml", "tracy"}
            links {"box2d", "pugixml", "tracy", "SDL3", "SDL3_image", "SDL3_ttf", "jpeg", "libpng"}
            links {"avcodec", "avformat", "avutil", "swscale", "swresample"}
            -- Discord Social SDK: link directly to the partner library binary
            links { "discord_partner_sdk" }
            characterset ("Unicode")
            buildoptions { "/Zc:__cplusplus" }

        filter "system:windows"
            defines{"_WIN32"}
            links {"winmm", "gdi32", "opengl32"}
            
            -- SDL3 x64 específic
            filter { "system:windows", "platforms:x64" }
                libdirs { "../bin/%{cfg.buildcfg}", sdl3_dir .. "/lib/x64", sdl3_image_dir .. "/lib/x64", sdl3_ttf_dir .. "/lib/x64", libjpeg_turbo_dir .. "/lib", libpng_dir .. "/lib", ffmpeg_dir .. "/lib" }
                -- Discord Social SDK
                libdirs { discord_sdk_dir .. "/lib/release" }
                links { "discord_partner_sdk" }
                postbuildcommands {
                    'xcopy /Y /D "$(SolutionDir)build\\external\\SDL3\\lib\\x64\\SDL3.dll" "$(SolutionDir)bin\\%{cfg.buildcfg}\\" 2>nul || exit 0',
                    'xcopy /Y /D "$(SolutionDir)build\\external\\SDL3_image\\lib\\x64\\SDL3_image.dll" "$(SolutionDir)bin\\%{cfg.buildcfg}\\" 2>nul || exit 0',
                    'xcopy /Y /D "$(SolutionDir)build\\external\\SDL3_ttf\\lib\\x64\\SDL3_ttf.dll" "$(SolutionDir)bin\\%{cfg.buildcfg}\\" 2>nul || exit 0',
                    'xcopy /Y /D "$(SolutionDir)build\\external\\libjpeg-turbo\\bin\\jpeg62.dll" "$(SolutionDir)bin\\%{cfg.buildcfg}\\" 2>nul || exit 0',
                    'xcopy /Y /D "$(SolutionDir)build\\external\\ffmpeg\\bin\\*.dll" "$(SolutionDir)bin\\%{cfg.buildcfg}\\" 2>nul || exit 0',
                    'xcopy /Y /D "$(SolutionDir)build\\external\\discord_social_sdk\\bin\\release\\discord_partner_sdk.dll" "$(SolutionDir)bin\\%{cfg.buildcfg}\\" 2>nul || exit 0'
                }

            filter { "system:linux" }
                libdirs { discord_sdk_dir .. "/lib/release" }
                links { "discord_partner_sdk" } -- Links to libdiscord_partner_sdk.so

            filter { "system:macosx" }
                libdirs { discord_sdk_dir .. "/lib/release" }
                links { "discord_partner_sdk" } -- Links to libdiscord_partner_sdk.dylib
        filter "system:linux"
            links {"pthread", "m", "dl", "rt", "X11"}

        filter{}

    project "box2d"
        kind "StaticLib"
        
        location "build_files/"
        
        language "C"
        targetdir "../bin/%{cfg.buildcfg}"
        
        -- Use C11 standard for static_assert support
        cdialect "C11"
        
        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            characterset ("Unicode")
            -- MSVC compatibility: Force C compilation
            buildoptions { "/TC" }
            -- For MSVC, define _Static_assert as a no-op since MSVC doesn't fully support C11
            defines { "_Static_assert(x,y)=" }
        
        filter "system:not windows"
            -- For non-Windows systems, ensure C11 support and define required macros
            buildoptions { "-std=c11" }
            defines { "_GNU_SOURCE", "_POSIX_C_SOURCE=200809L" }
        
        filter{}
        
        includedirs {box2d_dir, box2d_dir .. "/include", box2d_dir .. "/src", box2d_dir .. "/extern/simde" }
        vpaths
        {
            ["Header Files"] = { box2d_dir .. "/include/**.h", box2d_dir .. "/src/**.h"},
            ["Source Files/*"] = { box2d_dir .. "/src/**.c"},
        }
        files {box2d_dir .. "/include/**.h", box2d_dir .. "/src/**.c", box2d_dir .. "/src/**.h"}
        
        filter{}

    project "pugixml"
        kind "StaticLib"
        
        location "build_files/"
        
        language "C++"
        targetdir "../bin/%{cfg.buildcfg}"
        
        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            characterset ("Unicode")
        filter{}
        
        includedirs { pugixml_dir .. "/src" }
        
        files { 
            pugixml_dir .. "/src/pugixml.cpp",
            pugixml_dir .. "/src/pugixml.hpp",
            pugixml_dir .. "/src/pugiconfig.hpp"
        }
        
        filter{}

    project "tracy"
        kind "StaticLib"
        
        location "build_files/"
        
        language "C++"
        cppdialect "C++17"
        targetdir "../bin/%{cfg.buildcfg}"
        
        -- TRACY_ENABLE activates profiling; remove this define to compile out all Tracy calls
        defines { "TRACY_ENABLE" }
        
        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            characterset ("Unicode")
            buildoptions { "/Zc:__cplusplus" }
        filter{}
        
        includedirs { tracy_dir .. "/public" }
        
        vpaths
        {
            ["Header Files"] = { tracy_dir .. "/public/tracy/**.hpp", tracy_dir .. "/public/tracy/**.h" },
            ["Source Files"] = { tracy_dir .. "/public/TracyClient.cpp" },
        }
        files {
            tracy_dir .. "/public/tracy/Tracy.hpp",
            tracy_dir .. "/public/tracy/TracyC.h",
            tracy_dir .. "/public/TracyClient.cpp"
        }
        
        filter "system:windows"
            links { "ws2_32", "dbghelp", "advapi32", "user32" }
        
        filter "system:linux"
            links { "pthread", "dl" }

        filter {}

