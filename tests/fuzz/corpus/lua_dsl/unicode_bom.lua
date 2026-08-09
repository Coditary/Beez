-- UTF-8 BOM prefix
step({
    name = "über-café-日本語",
    phase = "demo",
    scope = "i18n",
    run = "echo unicode",
    description = "line1\nline2\t\"quoted\"",
})
workflow("run", { { phase = "demo", scope = "i18n" } })
