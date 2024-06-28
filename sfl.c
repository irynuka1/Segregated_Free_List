// Coitu Sebastian-Teodor 314CA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DIE(assertion, call_description)					   \
	do {                                                       \
		if (assertion) {                                       \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__); \
			perror(call_description);                          \
			exit(EXIT_FAILURE);                                \
		}                                                      \
	} while (0)

typedef struct block_t block_t;
struct block_t {
	void *data;
	block_t *next;
	block_t *prev;
};

typedef struct info_t info_t;
struct info_t {
	unsigned int address;
	unsigned int size;
	char *str;
};

typedef struct dll_list_t dll_list_t;
struct dll_list_t {
	block_t *head;
	block_t *tail;
	unsigned int no_blocks;
	unsigned int block_size;
};

typedef struct heap_t heap_t;
struct heap_t {
	unsigned int start;
	unsigned int no_lists;
	unsigned int max_no_lists;
	unsigned int list_size;
	dll_list_t *lists;
};

void dll_add_nth_node(dll_list_t *list, unsigned int n, unsigned int address,
					  unsigned int block_size)
{
	if (n >= list->no_blocks)
		n = list->no_blocks;

	block_t *new_node = malloc(sizeof(block_t));
	DIE(!new_node, "malloc failed");
	new_node->data = malloc(sizeof(info_t));
	DIE(!new_node->data, "malloc failed");
	info_t *info = (info_t *)new_node->data;
	info->str = malloc(block_size);
	DIE(!info->str, "malloc failed");

	info->size = block_size;
	info->address = address;

	if (list->no_blocks == 0) {
		new_node->next = NULL;
		new_node->prev = NULL;
		list->head = new_node;
		list->tail = new_node;

	} else if (n == 0) {
		new_node->next = list->head;
		list->head->prev = new_node;
		list->head = new_node;
		list->head->prev = NULL;

	} else if (n == list->no_blocks) {
		new_node->prev = list->tail;
		new_node->next = NULL;
		list->tail->next = new_node;
		list->tail = new_node;

	} else {
		block_t *current = list->head;
		DIE(!current, "malloc failed");
		for (unsigned int i = 0; i < n; i++)
			current = current->next;

		new_node->next = current;
		new_node->prev = current->prev;
		current->prev->next = new_node;
		current->prev = new_node;
	}

	list->no_blocks++;
}

// Functia adauga un block in lista de blocuri alocate
void dll_add_allocated_node(dll_list_t *list, block_t *block,
							unsigned int size, unsigned int n)
{
	info_t *info = (info_t *)block->data;
	info->size = size;

	if (list->no_blocks == 0) {
		block->next = NULL;
		block->prev = NULL;
		list->head = block;
		list->tail = block;

	} else if (n == 0) {
		block->next = list->head;
		list->head->prev = block;
		list->head = block;
		list->head->prev = NULL;

	} else if (n == list->no_blocks) {
		block->prev = list->tail;
		block->next = NULL;
		list->tail->next = block;
		list->tail = block;

	} else {
		block_t *current = list->head;
		DIE(!current, "malloc failed");
		for (unsigned int i = 0; i < n; i++)
			current = current->next;

		block->next = current;
		block->prev = current->prev;
		current->prev->next = block;
		current->prev = block;
	}

	list->no_blocks++;
}

void dll_remove_nth_node(dll_list_t *list, unsigned int n)
{
	if (n == 0) {
		list->head = list->head->next;
		if (list->head)
			list->head->prev = NULL;
		else
			list->tail = NULL;
	} else if (n >= list->no_blocks - 1) {
		list->tail = list->tail->prev;
		if (list->tail)
			list->tail->next = NULL;
		else
			list->head = NULL;
	} else {
		block_t *current = list->head;
		for (unsigned int i = 0; i < n; i++)
			current = current->next;

		current->prev->next = current->next;
		current->next->prev = current->prev;
	}

	list->no_blocks--;
}

// Initializarea listei de blocuri
void init_list(unsigned int block_size, dll_list_t *list)
{
	list->head = NULL;
	list->tail = NULL;
	list->no_blocks = 0;
	list->block_size = block_size;
}

// Crearea heap-ului
heap_t *init_heap(unsigned int start, unsigned int no_lists,
				  unsigned int list_size)
{
	heap_t *heap = (heap_t *)malloc(sizeof(heap_t));
	DIE(!heap, "malloc failed");
	heap->start = start;
	heap->no_lists = no_lists;
	heap->max_no_lists = no_lists;
	heap->list_size = list_size;
	heap->lists = (dll_list_t *)malloc(no_lists * sizeof(dll_list_t));
	DIE(!heap->lists, "malloc failed");

	int block_size = 8;
	unsigned int address = start;
	for (unsigned int i = 0; i < no_lists; i++) {
		init_list(block_size, &heap->lists[i]);

		// Adaugarea blocurilor in lista si atribuirea de adrese
		for (unsigned int j = 0; j < list_size / block_size; j++) {
			dll_add_nth_node(&heap->lists[i], heap->lists[i].no_blocks,
							 address, block_size);
			address += block_size;
		}

		block_size *= 2;
	}

	return heap;
}

// Sortarea vectorului de liste in functie de dimensiunea blocurilor
void sort_array(heap_t *heap)
{
	for (unsigned int i = 0; i < heap->no_lists - 1; i++)
		for (unsigned int j = i + 1; j < heap->no_lists; j++)
			if (heap->lists[i].block_size > heap->lists[j].block_size) {
				dll_list_t temp = heap->lists[i];
				heap->lists[i] = heap->lists[j];
				heap->lists[j] = temp;
			}
}

/*
 * Returneaaza pozitia pe care trebuie adaugat un block in lista de blocuri
 * libere in functie de adresa acestuia.
 */
int get_add_pos(dll_list_t *list, unsigned int address)
{
	int pos = 0;
	block_t *current = list->head;
	info_t *info;

	while (current) {
		info = (info_t *)current->data;
		if (info->address > address)
			return pos;
		pos++;
		current = current->next;
	}

	return pos;
}

void realloc_heap(heap_t *heap, unsigned int new_no_lists)
{
	heap->lists = (dll_list_t *)realloc(heap->lists,
										new_no_lists * sizeof(dll_list_t));
	DIE(!heap->lists, "realloc failed");
	heap->max_no_lists = new_no_lists;
}

// Functia aloca un block de memorie
void heap_malloc(heap_t *heap, unsigned int size, dll_list_t *allocated_list,
				 unsigned int *no_mallocs, unsigned int *no_frag)
{
	/* DACA GASESC UN BLOCK DE DIMENSIUNE SIZE */
	if (size <= heap->list_size) {
		for (unsigned int i = 0; i < heap->no_lists; i++)
			if (heap->lists[i].block_size == size &&
				heap->lists[i].no_blocks > 0) {
				block_t *current = heap->lists[i].head;
				dll_remove_nth_node(&heap->lists[i], 0);
				info_t *curr_info = (info_t *)current->data;
				unsigned int pos = get_add_pos(allocated_list,
												curr_info->address);
				dll_add_allocated_node(allocated_list, current, size, pos);
				(*no_mallocs)++;
				return;
			}

		/* DACA NU GASESC UN BLOCK DE DIMENSIUNE SIZE - FRAGMENTARE */
		int list_index = -1;
		for (unsigned int i = 0; i < heap->no_lists; i++)
			if (heap->lists[i].block_size >= size &&
				heap->lists[i].no_blocks > 0) {
				list_index = i;
				break;
			}

		if (list_index == -1) {
			printf("Out of memory\n");
			return;
		}

		// Scot block-ul din lista de blocuri libere
		info_t *info = (info_t *)heap->lists[list_index].head->data;
		unsigned int address = info->address;
		unsigned int block_size = heap->lists[list_index].block_size;
		free(info->str);
		free(info);
		block_t *removed = heap->lists[list_index].head;
		dll_remove_nth_node(&heap->lists[list_index], 0);
		free(removed);

		// Adaug block-ul in lista de alocate cu noile date
		block_t *current = malloc(sizeof(block_t));
		DIE(!current, "malloc failed");
		current->data = malloc(sizeof(info_t));
		DIE(!current->data, "malloc failed");
		info_t *curr_info = (info_t *)current->data;
		curr_info->str = malloc(size);
		DIE(!curr_info->str, "malloc failed");
		curr_info->address = address;
		unsigned int pos = get_add_pos(allocated_list, curr_info->address);
		dll_add_allocated_node(allocated_list, current, size, pos);
		(*no_mallocs)++;
		(*no_frag)++;

		// Adaug block-ul ramas intr-o lista de blocuri libere
		for (unsigned int i = 0; i < heap->no_lists; i++)
			if (heap->lists[i].block_size == block_size - size) {
				unsigned int pos = get_add_pos(&heap->lists[i],
											   address + size);
				dll_add_nth_node(&heap->lists[i], pos, address + size,
								 block_size - size);
				return;
			}

		if (heap->no_lists == heap->max_no_lists) {
			realloc_heap(heap, heap->no_lists * 2);
			DIE(!heap->lists, "realloc failed");
		}

		init_list(block_size - size, &heap->lists[heap->no_lists++]);
		dll_add_nth_node(&heap->lists[heap->no_lists - 1], 0, address + size,
						 block_size - size);
		sort_array(heap);
		return;
	}

	printf("Out of memory\n");
}

// Functia elibereaza un block alocat
void free_allocated_block(heap_t *heap, dll_list_t *allocated_list,
						  unsigned int address, unsigned int *no_frees)
{
	int ok = 0, index = 0;
	block_t *current = allocated_list->head;

	/*
	 * Caut block-ul in lista de blocuri alocate.
	 * Daca il gasesc, il scot din lista de alocate si il adaug in lista de
	 * blocuri libere.
	 * Daca este nevoie creez o noua lista de blocuri libere.
	 */
	while (current) {
		info_t *info = (info_t *)current->data;
		if (info->address == address) {
			for (unsigned int i = 0; i < heap->no_lists; i++)
				if (heap->lists[i].block_size == info->size) {
					free(info->str);
					unsigned int pos = get_add_pos(&heap->lists[i], address);
					dll_add_nth_node(&heap->lists[i], pos, address,
									 info->size);
					ok = 1;
					break;
				}

			if (!ok) {
				free(info->str);
				if (heap->no_lists == heap->max_no_lists) {
					realloc_heap(heap, heap->no_lists * 2);
					DIE(!heap->lists, "realloc failed");
				}

				init_list(info->size, &heap->lists[heap->no_lists++]);
				dll_add_nth_node(&heap->lists[heap->no_lists - 1], 0,
								 address, info->size);
				sort_array(heap);
			}

			dll_remove_nth_node(allocated_list, index);
			free(info);
			free(current);
			(*no_frees)++;
			return;
		}

		current = current->next;
		index++;
	}

	if (address == 0)
		return;

	printf("Invalid free\n");
}

// Functie de print pentru blocurile libere
void free_blocks_printf(heap_t *heap)
{
	for (unsigned int i = 0; i < heap->no_lists; i++) {
		if (heap->lists[i].no_blocks > 0) {
			printf("Blocks with %d bytes - %d free block(s) : ",
				   heap->lists[i].block_size, heap->lists[i].no_blocks);
			block_t *current = heap->lists[i].head;

			while (current) {
				info_t *info = (info_t *)current->data;
				if (!current->next)
					printf("0x%x", info->address);
				else
					printf("0x%x ", info->address);

				current = current->next;
			}
			printf("\n");
		}
	}
}

// Functie de print pentru blocurile alocate
void allocated_blocks_printf(dll_list_t *allocated_list)
{
	if (allocated_list->no_blocks == 0) {
		printf("Allocated blocks :\n");
		return;
	}

	printf("Allocated blocks : ");
	block_t *current = allocated_list->head;

	while (current) {
		info_t *info = (info_t *)current->data;
		if (!current->next)
			printf("(0x%x - %d)", info->address, info->size);
		else
			printf("(0x%x - %d) ", info->address, info->size);

		current = current->next;
	}
	printf("\n");
}

void dump_memory(heap_t *heap, dll_list_t *allocated_list,
				 unsigned int no_mallocs, unsigned int no_frag,
				 unsigned int no_frees)
{
	// Calculez memoria libera
	printf("+++++DUMP+++++\n");
	unsigned long free_memory = 0, allocated_memory = 0;
	for (unsigned int i = 0; i < heap->no_lists; i++)
		free_memory += heap->lists[i].no_blocks * heap->lists[i].block_size;

	// Calculez memoria alocata
	block_t *current = allocated_list->head;
	while (current) {
		info_t *info = (info_t *)current->data;
		allocated_memory += info->size;
		current = current->next;
	}

	printf("Total memory: %lu bytes\n", free_memory + allocated_memory);
	printf("Total allocated memory: %lu bytes\n", allocated_memory);
	printf("Total free memory: %lu bytes\n", free_memory);

	// Calculez numarul de blocuri libere
	unsigned int no_free_blocks = 0;
	for (unsigned int i = 0; i < heap->no_lists; i++)
		no_free_blocks += heap->lists[i].no_blocks;

	printf("Free blocks: %d\n", no_free_blocks);
	printf("Number of allocated blocks: %d\n", allocated_list->no_blocks);
	printf("Number of malloc calls: %d\n", no_mallocs);
	printf("Number of fragmentations: %d\n", no_frag);
	printf("Number of free calls: %d\n", no_frees);
	free_blocks_printf(heap);
	allocated_blocks_printf(allocated_list);
	printf("-----DUMP-----\n");
}

// Functia distruge heap-ul si lista de blocuri alocate
void drestroy_heap(heap_t *heap, dll_list_t *allocated_list)
{
	for (unsigned int i = 0; i < heap->no_lists; i++) {
		block_t *current = heap->lists[i].head;
		while (current) {
			block_t *temp = current;
			info_t *info = (info_t *)temp->data;
			current = current->next;
			free(info->str);
			free(info);
			free(temp);
		}
	}

	free(heap->lists);
	free(heap);
	block_t *current = allocated_list->head;

	while (current) {
		block_t *temp = current;
		info_t *info = (info_t *)temp->data;
		current = current->next;
		free(info->str);
		free(info);
		free(temp);
	}

	free(allocated_list);
}

// Se scrie un string in memoria alocata
int write(dll_list_t *list, unsigned int address, char string[],
		  unsigned long size)
{
	if (size > strlen(string))
		size = strlen(string);

	int ok = 0;
	// Verific daca adresa este in lista de blocuri alocate
	block_t *current = list->head;
	while (current) {
		info_t *info = (info_t *)current->data;
		if (address == info->address) {
			ok = 1;
			break;
		}

		current = current->next;
	}
	block_t *temp = current;

	// Verific daca exista o zona continua de memorie suficienta
	unsigned int temp_address = address;
	if (ok) {
		while (temp_address < address + size) {
			info_t *temp_info = (info_t *)temp->data;

			if (temp_address >= temp_info->address &&
				temp_address < temp_info->address + temp_info->size) {
				temp_address++;
			} else {
				if (temp->next) {
					temp = temp->next;
				} else {
					printf("Segmentation fault (core dumped)\n");
					return 0;
				}
			}
		}
	} else {
		printf("Segmentation fault (core dumped)\n");
		return 0;
	}

	// Scriu string-ul in memoria alocata
	if (ok) {
		unsigned int i = 0;
		while (i < size) {
			info_t *info = (info_t *)current->data;
			if (address >= info->address &&
				address < info->address + info->size) {
				*(info->str + address - info->address) = string[i];
				i++;
				address++;
			} else {
				current = current->next;
			}
		}
	} else {
		printf("Segmentation fault (core dumped)\n");
		return 0;
	}

	return 1;
}

// Se citeste un string din memoria alocata
int read(dll_list_t *list, unsigned int address, int size)
{
	info_t *info;
	int ok = 0;
	// Verific daca adresa exista in lista de blocuri alocate
	block_t *current = list->head;
	while (current) {
		info = (info_t *)current->data;
		if (address >= info->address &&
			address < info->address + info->size) {
			ok = 1;
			break;
		}

		current = current->next;
	}

	// Verific daca exista o zona continua de memorie suficienta
	block_t *temp = current;

	unsigned int temp_address = address;
	if (ok) {
		while (temp_address < address + size) {
			info_t *temp_info = (info_t *)temp->data;
			if (temp_address >= temp_info->address &&
				temp_address < temp_info->address + temp_info->size) {
				temp_address++;
			} else {
				if (temp->next) {
					temp = temp->next;
				} else {
					printf("Segmentation fault (core dumped)\n");
					return 0;
				}
			}
		}
	} else {
		printf("Segmentation fault (core dumped)\n");
		return 0;
	}

	// Citesc string-ul din memoria alocata
	if (current) {
		int i = 0;
		while (i < size) {
			info = (info_t *)current->data;
			if (address >= info->address &&
				address < info->address + info->size) {
				printf("%c", *(info->str + address -
				info->address));
				i++;
				address++;
			} else {
				current = current->next;
			}
		}
		printf("\n");
	} else {
		printf("Segmentation fault (core dumped)\n");
	}

	return 1;
}

int main(void)
{
	char command[100];
	bool running = true;
	unsigned int no_mallocs = 0, no_frees = 0, no_frag = 0;
	heap_t *heap;

	// Initializarea listei de blocuri alocate
	dll_list_t *allocated_list = malloc(sizeof(dll_list_t));
	DIE(!allocated_list, "malloc failed");
	init_list(0, allocated_list);

	while (running) {
		scanf("%s", command);
		if (strcmp(command, "INIT_HEAP") == 0) {
			unsigned int start, no_lists, list_size, reconstruct_type;
			scanf("%x %d %d %d", &start, &no_lists, &list_size,
				  &reconstruct_type);

			heap = init_heap(start, no_lists, list_size);

		} else if (strcmp(command, "MALLOC") == 0) {
			unsigned int size;
			scanf("%d", &size);
			heap_malloc(heap, size, allocated_list, &no_mallocs, &no_frag);

		} else if (strcmp(command, "FREE") == 0) {
			unsigned int address;
			scanf("%x", &address);
			free_allocated_block(heap, allocated_list, address, &no_frees);

		} else if (strcmp(command, "DUMP_MEMORY") == 0) {
			dump_memory(heap, allocated_list, no_mallocs, no_frag, no_frees);

		} else if (strcmp(command, "DESTROY_HEAP") == 0) {
			drestroy_heap(heap, allocated_list);
			running = false;

		} else if (strcmp(command, "WRITE") == 0) {
			unsigned int address;
			unsigned long size;
			int index = 0;
			char pos;
			char string[600];
			scanf("%x ", &address);
			scanf("%c", &pos); // Citesc prima ghilimea
			scanf("%c", &pos); // Citesc primmul caracter din string
			while (pos != '\"') {
				string[index++] = pos;
				scanf("%c", &pos);
			}
			string[index] = '\0';
			scanf("%ld", &size);

			int ok = write(allocated_list, address, string, size);
			if (!ok) {
				dump_memory(heap, allocated_list, no_mallocs,
							no_frag, no_frees);
				drestroy_heap(heap, allocated_list);
				running = false;
			}

		} else if (strcmp(command, "READ") == 0) {
			unsigned int address, size;
			scanf("%x %d", &address, &size);

			int ok = read(allocated_list, address, size);
			if (!ok) {
				dump_memory(heap, allocated_list, no_mallocs,
							no_frag, no_frees);
				drestroy_heap(heap, allocated_list);
				running = false;
			}
		}
	}

	return 0;
}
