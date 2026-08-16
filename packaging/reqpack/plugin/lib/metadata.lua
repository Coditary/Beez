local util = dofile((debug.getinfo(1, "S").source:gsub("^@", "")):match("^(.*)/") .. "/util.lua")

local M = {}

local VENDOR = "Coditary"
local MAINTAINER_EMAIL = "Matographo@gmail.com"
local HOMEPAGE = "https://github.com/Coditary/Beez"

local SYSTEM_BY_PLATFORM = {
  linux = "linux",
  macos = "macos",
}

function M.package_slug(organization, name)
  return organization .. "-" .. name
end

function M.archive_name(organization, name, version, platform, arch)
  return string.format(
    "%s-%s-%s-%s.rqp",
    M.package_slug(organization, name),
    version,
    platform,
    arch
  )
end

function M.build_metadata(options)
  local version = util.normalized_version(options.version)
  local platform = options.platform
  local arch = options.arch
  local archive_name = M.archive_name(options.organization, options.name, version, platform, arch)
  local release_tag = "v" .. version
  local source_url = options.source_url or HOMEPAGE
  local qualified = options.organization .. "/" .. options.name

  return {
    formatVersion = 1,
    name = qualified,
    organization = options.organization,
    plugin = options.name,
    version = version,
    release = 1,
    revision = 0,
    summary = options.summary or ("Beez plugin " .. qualified),
    description = options.description or ("Beez workflow plugin " .. qualified),
    license = options.license or "Apache-2.0",
    architecture = arch,
    system = { SYSTEM_BY_PLATFORM[platform] },
    vendor = VENDOR,
    maintainerEmail = MAINTAINER_EMAIL,
    url = source_url .. "/releases/download/" .. release_tag .. "/" .. archive_name,
    homepage = options.homepage or HOMEPAGE,
    sourceUrl = options.homepage or HOMEPAGE,
    buildDate = os.date("!%Y-%m-%dT%H:%M:%SZ"),
    tags = options.tags or { "beez", "plugin", options.organization, options.name },
    payload = {
      path = "payload/payload.tar.zst",
      archive = "tar",
      compression = "zstd",
      hashAlgorithm = "sha256",
      hashFile = "hashes/payload.sha256",
      sizeCompressed = options.size_compressed,
      sizeInstalledExpected = options.size_installed,
    },
  }
end

function M.render_template(template_path, output_path, metadata)
  local template = util.read_file(template_path)
  if template == nil then
    error("metadata template not found: " .. template_path)
  end

  local payload = metadata.payload
  local replacements = {
    ["@QUALIFIED_NAME@"] = string.format("%q", metadata.name),
    ["@ORGANIZATION@"] = string.format("%q", metadata.organization),
    ["@PLUGIN_NAME@"] = string.format("%q", metadata.plugin),
    ["@VERSION@"] = string.format("%q", metadata.version),
    ["@ARCHITECTURE@"] = string.format("%q", metadata.architecture),
    ["@SYSTEM@"] = string.format("%q", metadata.system[1]),
    ["@URL@"] = string.format("%q", metadata.url),
    ["@BUILD_DATE@"] = string.format("%q", metadata.buildDate),
    ["@SUMMARY@"] = string.format("%q", metadata.summary),
    ["@DESCRIPTION@"] = string.format("%q", metadata.description),
    ["@SIZE_COMPRESSED@"] = tostring(payload.sizeCompressed),
    ["@SIZE_INSTALLED_EXPECTED@"] = tostring(payload.sizeInstalledExpected),
  }

  for key, value in pairs(replacements) do
    template = template:gsub(key, value, 1)
  end

  local ok, err = util.write_file(output_path, template)
  if not ok then
    error("failed to write metadata: " .. tostring(err))
  end
end

return M
