#ifndef SQV_STYLE_HEADER_
#define SQV_STYLE_HEADER_

local const struct {
  struct {
    struct {
      U32 size;
      struct {
        U8 r;
        U8 g;
        U8 b;
      } color;
    } border;
  } general;
  struct {
    struct {
      U32 x;
      U32 y;
    } padding;
    struct {
      struct {
        U8 r;
        U8 g;
        U8 b;
      } background;
    } color;
  } window;

  struct {
    U32 height;
    struct {
      U32 height;
    } icon;
  } toolbar;

  struct {
    U32 height;
  } statusbar;

  struct {
    struct {
      U32 x;
      U32 y;
    } size;
  } scrollbar;

  struct {
    struct {
      struct {
        struct {
          U8 r;
          U8 g;
          U8 b;
          U8 a;
        } background;
      } color;
      struct {
        U32 width;
        U32 height;
      } image;
      struct {
        U32 width;
        U32 height;
      } text;
      U32 gap;
    } icon;
    struct {
      U32 x;
      U32 y;
    } padding;
    struct {
      U32 w;
      U32 h;
    } contextual;
    struct {
      struct {
        U8 r;
        U8 g;
        U8 b;
        U8 a;
      } background;
    } color;
  } explorer;

  struct {
    struct {
      U32 x;
      U32 y;
      U32 w;
      U32 h;
    } rectangle;
    struct {
      struct {
        U32 r;
        U32 g;
        U32 b;
      } color;
    } background;
    struct {
      U32 height;
    }font;
  } dialog;

  struct {
    struct {
      struct {
        U8 r;
        U8 g;
        U8 b;
      } normal;
      struct {
        U8 r;
        U8 g;
        U8 b;
      } hover;
      struct {
        U8 r;
        U8 g;
        U8 b;
      } active;
    } color;
  } button;

  struct {
    U32 height;
  } font;

} STYLE = {
    .general =
        {
            .border =
                {
                    .size = 0,
                    .color =
                        {
                            .r = 255,
                            .g = 0,
                            .b = 0,
                        },
                },
        },
    .window =
        {
            .padding =
                {
                    .x = 2,
                    .y = 2,
                },
            .color =
                {
                    .background =
                        {
                            .r = 60,
                            .g = 60,
                            .b = 60,
                        },
                },
        },
    .toolbar =
        {
            .height = 45,
            .icon =
                {
                    .height = 40,
                },
        },
    .statusbar =
        {
            .height = 35,
        },
    .scrollbar =
        {
            .size =
                {
                    .x = 2,
                    .y = 2,
                },
        },
    .explorer =
        {
            .icon =
                {
                    .image =
                        {
                            .width = 50,
                            .height = 50,
                        },
                    .text =
                        {
                            .width = 160,
                            .height = 50,
                        },
                    .gap = 2,
                    .color =
                        {
                            .background =
                                {
                                    .r = 34,
                                    .g = 37,
                                    .b = 35,
                                    .a = 255,
                                },
                        },
                },
            .color =
                {
                    .background =
                        {
                            .r = 30,
                            .g = 30,
                            .b = 30,
                            .a = 255,
                        },
                },
            .padding =
                {
                    .x = 2,
                    .y = 2,
                },
            .contextual =
                {
                    .w = 150,
                    .h = 230,
                },
        },
    .dialog =
        {
            .rectangle =
                {
                    .x = 50,
                    .y = 50,
                    .w = 100,  // this is the delta
                    .h = 190,
                },
            .background =
                {
                    .color =
                        {
                            .r = 60,
                            .g = 60,
                            .b = 60,
                        },
                },
            .font = {
              .height = 25,
            },
        },
    .button =
        {
            .color =
                {
                    .normal =
                        {
                            .r = 100,
                            .g = 100,
                            .b = 100,
                        },
                    .hover =
                        {
                            .r = 150,
                            .g = 150,
                            .b = 150,
                        },
                    .active =
                        {
                            .r = 200,
                            .g = 200,
                            .b = 200,
                        },
                },
        },

};

#endif /* SQV_STYLE_HEADER_ */
