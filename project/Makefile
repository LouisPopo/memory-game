SERVER_OBJS = board_library.c server-memory.c
PLAYER_OBJS = UI_library.c player-memory.c
BOT_OBJS = UI_library.c bot-memory.c

CC = gcc

SERVER_NAME = server
PLAYER_NAME = player
BOT_NAME = bot

LINKER_FLAGS = -lSDL2 -lSDL2_ttf -lpthread -w

all: server client bot

server: $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) $(LINKER_FLAGS) -o $(SERVER_NAME)

client : $(PLAYER_OBJS)
	$(CC) $(PLAYER_OBJS) $(LINKER_FLAGS) -o $(CLIENT_NAME)

bot : $(BOT_OBJS)
	$(CC) $(BOT_OBJS) $(LINKER_FLAGS) -o $(BOT_NAME)

SINGLE_OBJS = board_library.c UI_library.c memory-single.c
SINGLE_NAME = memory_single

single : $(SINGLE_OBJS)
	$(CC) $(SINGLE_OBJS) $(LINKER_FLAGS) -o $(SINGLE_NAME)

clean : 
	rm $(SERVER_NAME) $(CLIENT_NAME) $(BOT_NAME)
