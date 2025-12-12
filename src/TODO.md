# TODO

- [X] find why some .PAK files fail ro cause crash
- [X] add context menu to files/folders
- [X] add delete file/folder feature
- [X] not load file content
- [X] show proper node name, instead of 'foo/bar/baz.txt' show 'baz.txt'
- [X] add a profiling build using tracy!
- [X] maybe open the file and read from it as like a stream and keep the file open until the end!
- [ ] allow droping files even if there is already file loaded
- [ ] lazy load file contents when needed! if ever!
- [ ] extract file/foler
- [ ] add folder drag into an existing loaded pak to create new .PAK file

# CHALLENGING
- [!] use Str8 everywhere it makes sense:
      this requires some annoying code change and i still am
      not sure about the benefits!

# REJECTED
- [!] edit mode:
      let's keep this tool simple, we only have these functionalities:
      - view and extract items
      - and create from scratch
- [!] add save current pak as another name:
      same as above
