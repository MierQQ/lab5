#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CodedTree {
	unsigned char value;
	int type;
} CodedTree;

typedef struct Tree {
	unsigned char value;
	struct Tree* left;
	struct Tree* right;
} Tree;

typedef Tree* PTree;

typedef struct Node {
	int count;
	PTree value;
	struct Node* next;
} Node;

typedef Node* PNode;

typedef struct CodeChar {
	unsigned char code[256][32];
	CodedTree codedTree[1024];
	int size;
} CodeChar;

typedef struct BitWriter {
	int writePosition;
	unsigned char buffer;
	FILE* output;
} BitWriter;

typedef struct BitReader {
	FILE* input;
	unsigned char buffer;
	long long int position;
	long long int endBits;
} BitReader;

void WriteBit(char bit, BitWriter* writer) {
	unsigned char bitPosition = 128;
	bitPosition >>= (writer->writePosition % 8);
	bitPosition = bit ? bitPosition : 0;
	writer->buffer |= bitPosition;
	++writer->writePosition;
	if (!(writer->writePosition % 8)) {
		fwrite(&writer->buffer, 1, 1, writer->output);
		writer->buffer = 0;
	}
}

void WriteChar(unsigned char ch, BitWriter* writer) {
	unsigned char i = 1;
	while (i) {
		WriteBit(((ch & i) ? 1 : 0), writer);
		i <<= 1;
	}
}

void DeleteTree(PTree tree) {
	if (tree) {
		DeleteTree(tree->left);
		DeleteTree(tree->right);
		free(tree);
	}
}

PTree CreateTree(char value, PTree left, PTree right) {
	PTree newTree = malloc(sizeof(Tree));
	if (!newTree) {
		abort();
	}
	newTree->value = value;
	newTree->left = left;
	newTree->right = right;
	return newTree;
}

void DeleteTreeChar(PTree tree) {
	if (tree) {
		DeleteTreeChar(tree->left);
		DeleteTreeChar(tree->right);
		free(tree);
	}
}


PNode Push(PTree value, int count, PNode head) {
	PNode newNode = malloc(sizeof(Node));
	if (!newNode) {
		abort();
	}
	newNode->count = count;
	newNode->next = head;
	newNode->value = value;
	return newNode;
}

PNode SortedPush(PTree value, int count, PNode head) {
	PNode itterator = head;
	if (!itterator) {
		return Push(value, count, itterator);
	}
	while (itterator->next && itterator->count < count) {
		itterator = itterator->next;
	}
	if (itterator->count < count) {
		itterator->next = Push(value, count, itterator->next);
	} else {
		PTree savedTree = itterator->value;
		itterator->value = value;
		itterator->next = Push(savedTree, count, itterator->next);
	}
	return head;
}

PTree Pop(PNode* head, int* count) {
	PTree savedTree = (*head)->value;
	PNode savedNode = (*head)->next;
	*count = (*head)->count;
	free(*head);
	*head = savedNode;
	return savedTree;
}

void FillCodeList(CodeChar* code, PTree tree, unsigned char* path, int height) {
	if (tree->left == NULL && tree->right == NULL) {
		path[height] = 0;
		code->codedTree[code->size].type = 0;
		code->codedTree[code->size++].value = 1;
		code->codedTree[code->size].type = 1;
		code->codedTree[code->size++].value = tree->value;
		strcpy((char*)code->code[(int)tree->value], (char*)path);
		return;
	}
	code->codedTree[code->size].type = 0;
	code->codedTree[code->size++].value = 0;
	path[height] = 2;
	FillCodeList(code, tree->left, path, height + 1);
	path[height] = 1;
	FillCodeList(code, tree->right, path, height + 1);
}

void GetCode(CodeChar* result, const char* input) {
	FILE* inFile = fopen(input, "rb");
	fseek(inFile, 3, SEEK_SET);
	int countOfChar[256] = { 0 };
	while (1) {
		unsigned char readenChar = fgetc(inFile);
		if (feof(inFile)) {
			break;
		}
		countOfChar[readenChar]++;

	}
	PNode ListToCreateTree = NULL;
	for (int i = 0; i < 256; ++i) {
		if (countOfChar[i] != 0) {
			PTree newTree = CreateTree(i, NULL, NULL);
			ListToCreateTree = SortedPush(newTree, countOfChar[i], ListToCreateTree);
		}
	}
	if (ListToCreateTree) {
		while (ListToCreateTree->next) {
			int countLeft;
			int countRight;
			PTree left = Pop(&ListToCreateTree, &countLeft);
			PTree right = Pop(&ListToCreateTree, &countRight);
			PTree newSubTree = CreateTree(0, left, right);
			ListToCreateTree = SortedPush(newSubTree, countLeft + countRight, ListToCreateTree);
		}
		int count;
		PTree resultTree = Pop(&ListToCreateTree, &count);
		unsigned char path[512] = { 0 };
		FillCodeList(result, resultTree, path, 0);
		if (resultTree->left == NULL && resultTree->right == NULL) {
			result->code[(int)resultTree->value][0] = 2;
			result->code[(int)resultTree->value][1] = 0;
		}
		DeleteTree(resultTree);
	}
	fclose(inFile);
}

void Zip(const char* input, const char* output) {
	CodeChar code;
	code.size = 0;
	FILE* check = fopen(input, "rb");
	fseek(check, 3, SEEK_SET);
	fgetc(check);
	if (feof(check)) {
		fclose(check);
		return;
	}
	fclose(check);
	GetCode(&code, input);
	FILE* outFile = fopen(output, "wb");
	fclose(outFile);
	if (code.size) {
		FILE* inFile = fopen(input, "rb");
		FILE* outFile = fopen(output, "wb");
		BitWriter writer;
		writer.buffer = 0;
		writer.output = outFile;
		writer.writePosition = 0;
		fseek(inFile, 3, SEEK_SET);
		for (int i = 0; i < 3; ++i) {
			WriteBit(0, &writer);
		}
		for (int i = 0; i < code.size; ++i) {
			if (code.codedTree[i].type == 0) {
				WriteBit(code.codedTree[i].value, &writer);
			} else {
				WriteChar(code.codedTree[i].value, &writer);
			}
		}
		while (1) {
			unsigned char readenChar = fgetc(inFile);
			if (feof(inFile)) {
				break;
			}
			for (int i = 0; i < (int)strlen((char*)code.code[readenChar]); ++i) {
				WriteBit(code.code[readenChar][i] - 1, &writer);
			}
		}
		if (writer.writePosition % 8 != 0) {
			if (!writer.buffer) {
				writer.buffer = 1;
			}
			fwrite(&writer.buffer, 1, 1, writer.output);
		}
		fseek(writer.output, 0, SEEK_SET);
		unsigned char count = writer.writePosition % 8;
		writer.buffer = 0;
		writer.writePosition = 0;
		for (int i = 0; i < 3; ++i) {
			WriteBit((((count >> i)& (unsigned char)1)), &writer);
		}
		for (int i = 0; i < code.size; ++i) {
			if (code.codedTree[i].type == 0) {
				WriteBit(code.codedTree[i].value, &writer);
			} else {
				WriteChar(code.codedTree[i].value, &writer);
			}
		}
		fclose(inFile);
		fclose(outFile);
	}
}

int GetBit(BitReader* reader) {
	if (reader->position % 8 == 0) {
		reader->buffer = (unsigned char)fgetc(reader->input);
	}
	unsigned char mask = 128 >> reader->position % 8;
	int result = (reader->buffer & mask) ? 1 : 0;
	++reader->position;
	return result;
}

PTree BuildTree(BitReader* reader) {
	PTree tree = CreateTree(0, NULL, NULL);
	int bit = GetBit(reader);
	if (!bit) {
		tree->left = BuildTree(reader);
		tree->right = BuildTree(reader);
	} else {
		unsigned char codeChar = 0;
		for (int i = 0; i < 8; ++i) {
			unsigned char mask = (unsigned char)GetBit(reader) << i;
			codeChar += mask;
		}
		tree->value = codeChar;
	}
	return tree;
}

int IsEndOfBit(BitReader reader) {
	if (reader.position >= reader.endBits) {
		return 1;
	}
	return 0;
}

void UnZip(const char* input, const char* output) {
	BitReader reader;
	reader.input = fopen(input, "rb");
	fseek(reader.input, 3, SEEK_SET);
	fgetc(reader.input);
	if (feof(reader.input)) {
		FILE* out = fopen(output, "wb");
		fclose(out);
		fclose(reader.input);
		return;
	}
	fseek(reader.input, 3, SEEK_SET);
	reader.position = 0;
	reader.endBits = 0;
	for (int i = 0; i < 3; ++i) {
		reader.endBits += (long long int)GetBit(&reader) << i;
	}
	long long int position = ftell(reader.input);
	fseek(reader.input, 0, SEEK_END);
	long long int endPosition = ftell(reader.input);
	if (reader.endBits) {
		reader.endBits += (endPosition - 4) * 8;
	} else {
		reader.endBits += (endPosition - 3) * 8;
	}
	fseek(reader.input, position, SEEK_SET);
	PTree codeTree = BuildTree(&reader);
	PTree treeItter = codeTree;
	FILE* out = fopen(output, "wb");
	while (!IsEndOfBit(reader)) {
		int bit = GetBit(&reader);
		if (treeItter->left == NULL && treeItter->right == NULL) {
			fprintf(out, "%c", treeItter->value);
			treeItter = codeTree;
			continue;
		} else if (bit) {
			treeItter = treeItter->left;
		} else {
			treeItter = treeItter->right;
		}
		if (treeItter->left == NULL && treeItter->right == NULL) {
			fprintf(out, "%c", treeItter->value);
			treeItter = codeTree;
		}
	}
	DeleteTreeChar(codeTree);
	fclose(out);
	fclose(reader.input);
}

void DoHuffman(char mode, const char* input, const char* output) {
	switch (mode) {
	case 'c': Zip(input, output);
		break;
	case'd': UnZip(input, output);
		break;
	}
}

int main() {
	char modeOfHaffman;
	FILE* inFile = fopen("in.txt", "rb");
	if (!fscanf(inFile, "%c", &modeOfHaffman)) {
		fclose(inFile);
		return -1;
	}
	fclose(inFile);
	DoHuffman(modeOfHaffman, "in.txt", "out.txt");
	return 0;
}
