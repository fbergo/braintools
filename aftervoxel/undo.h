
#ifndef UNDO_H
#define UNDO_H 1

void undo_init();
void undo_reset();
void undo_pop();
void undo_push(int orient);
void undo_update();

#endif
