# To list file
# ls -1 | xargs -I % echo \"%\",

files = [
#TOP
"app_package.vhd",
"app.vhd",
]

modules = {
"local" : ["common","ddr3k7-core","wshexp-core"],
}
