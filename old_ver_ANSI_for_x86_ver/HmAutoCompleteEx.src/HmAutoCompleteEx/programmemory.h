#pragma once

int checkProgramMemoryImage(char*szPath);

void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);

const int iProgramMemorySearchSize = 0x30000; // まぁ将来秀丸が少々拡張されてもこのくらいで探せるハズ

