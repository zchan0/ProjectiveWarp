CC	= clang++
C		= cpp
H   = h

CFLAGS	= -g 
LFLAGS	= -g

ifeq ("$(shell uname)", "Darwin")
  LDFLAGS     = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else
  ifeq ("$(shell uname)", "Linux")
    LDFLAGS   = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm
  endif
endif

EXE			= warper
HFILES  = ImageIO/ImageIO.${H} ImageIO/Image.${H} Matrix/matrix.${H}
OBJS    = ImageIO/ImageIO.o ImageIO/Image.o Matrix/matrix.o

all:	${EXE}

warper:	${OBJS} warper.o
	${CC} ${LFLAGS} -o warper ${OBJS} warper.o ${LDFLAGS}

warper.o: warper.${C} ${HFILES}
	${CC} ${CFLAGS} -c warper.${C}

matrix.o: Matrix/matrix.${C} ${HFILES}
	${CC} ${CFLAGS} -c matrix.${C} 

Image.o: ImageIO/Image.${C} ${HFILES}
	${CC} ${CFLAGS} -c Image.${C}

ImageIO.o: ImageIO/ImageIO.${C} ${HFILES}
	${CC} ${CFLAGS} -c ImageIO.${C} 

clean:
	rm -f core.* *.o *~ ${EXE} ${OBJS}
