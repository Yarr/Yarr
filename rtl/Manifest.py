# To list file
# ls -1 | xargs -I % echo \"%\",

files = [
    #TOP
    "fe65p2_addon.vhd",
]

modules = {
    "local" : ["common",
        "i2c-master",
        "rx-core",
        "tx-core",
        "trigger-logic"],
}
