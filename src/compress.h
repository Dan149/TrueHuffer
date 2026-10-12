typedef struct {
  *char deflatedContent;
  *char inflatedContent;
  *char checksum;
} TextObject;

int inflate(*TextObject obj);
int deflate(*TextObject obj);
