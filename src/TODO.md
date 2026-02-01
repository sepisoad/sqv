# TODO
- [ ] create a generic tree data structur to abstract PAK and DIR
- [ ] package DIR into PAK
- [ ] replace the fixed size error buffer with Str8+Arena

# CHALLENGING
- [?] use Str8 everywhere it makes sense:
      i made some good progress on this

# REJECTED
- [-] edit mode:
      let's keep this tool simple, we only have these functionalities:
      - view and extract items
      - and create from scratch
- [-] add save current pak as another name:
      same as above

# DONE
- [X] find why some .PAK files fail ro cause crash
- [X] add context menu to files/folders
- [X] add delete file/folder feature
- [X] not load file content
- [X] show proper node name, instead of 'foo/bar/baz.txt' show 'baz.txt'
- [X] add a profiling build using tracy!
- [X] maybe open the file and read from it as like a stream and keep the file open until the end!
- [X] allow droping files even if there is already file loaded
- [X] extract file/foler
- [X] lazy load file contents when needed! if ever!
- [X] refactor sepi module
- [X] enumerate file system to get list of files and directory
