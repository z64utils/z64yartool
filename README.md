# `z64yartool`

Special thanks to [@Javarooster-png](https://github.com/Javarooster-png) for making recipes and helping test all the features! 🎉

`z64yartool` is operated using commands. You have the following commands at your disposal:
- `stat`
- `unyar`
- `dump`
- `build`

The commands are intended to be used in the above order. Let's walk through them.
## `stat`
This command reads a `.yar` file and prepares a recipe. Use it like so:
```
z64yartool stat icon_item_static.yar > icon_item_static.txt
```
The recipe is then written into `icon_item_static.txt`. We'll open that shortly.
## `unyar`
This command reads a `.yar` file and decompresses all of its contents, producing a new file with all the textures back-to-back. Here's an example:
```
z64yartool unyar icon_item_static.yar icon_item_static.bin
```
## putting `stat` and `unyar` together
If you open `icon_item_static.txt` in Notepad++, you'll notice that it contains addresses for each texture in the decompressed `.bin`:
```
icon_item_static.yar # relative path
icon_item_static/ # where the images live
??x??,unknown,00000000.png
??x??,unknown,00001000.png
??x??,unknown,00002000.png
??x??,unknown,00003000.png
??x??,unknown,00004000.png
```
Use `zztexview` to find out formats and dimensions! 🎉 You can plug in new info for each as you document things:
```
icon_item_static.yar # relative path
icon_item_static/ # where the images live
32x32,rgba32,OcarinaOfTime.png
32x32,rgba32,HerosBow.png
32x32,rgba32,FireArrow.png
32x32,rgba32,IceArrow.png
32x32,rgba32,LightArrow.png
etc
```
You don't have to fill in all the documentation in order to test. But they are expected to be in the proper order. If you want to test on an incomplete file, you can do so by deleting all undocumented entries.
Also, names can't have spaces.
## `dump`
You may have noticed in the recipe file that it has two paths at the top. These are:
- The name of the `.yar` file, relative to the recipe file.
- The name of the folder where the output images will be written, also relative.
- (You can probably use paths like `C:/wow/test` but this is untested.)

Dump by using the `dump` command like so. Note that the recipe file is the path given, not the `.yar` file:
```
z64yartool dump icon_item_static.txt
```
The recipe is what contains the dumping instructions, such as where the `.yar` file can be found (relative to the recipe file), to which directory the images should be written (also relative to the recipe file), the dimensions, formats, and filenames for each image.
## `build`
Build a recipe using the `build` command. It works identically to the `dump` command, but in reverse order. This means the `.yar` file referenced by the recipe will be overwritten. Keep backups in case you need them. Example usage:
```
z64yartool build icon_item_static.txt
```

