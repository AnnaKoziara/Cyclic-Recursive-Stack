/**
 * Dynamicznie ładowana biblioteka obsługująca rekurencyjne stosy.
 * 
 * Opis:
 * Biblioteka implementuje operacje na stosach, których elementami 
 * mogą być wartości liczbowe z zakresu uint64_t oraz wskaźniki do struktur
 * reprezentujących inne stosy.
 * 
 * Zliczanie referencji:
 * - Tuż po utworzeniu nowego stosu jego licznik referencji wynosi 1.
 * - Odłożenie stosu na inny stos zwiększa jego licznik o 1.
 * - Zdjęcie stosu ze stosu zmniejsza licznik o 1.
 * - Stos jest zwalniany z pamięci, gdy jego licznik referencji wyniesie 0.
 * 
 * Wykrywanie cykli:
 * - Biblioteka obsługuje usuwanie powstających cykli oraz izolowanych
 *  grafów między stosami, co zapobiega wyciekom pamięci.
 * 
 * Autor: Anna Koziara
 * ak479522@students.mimuw.edu.pl
**/

#include "rstack.h"
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>

//Typ, który określa czy dany węzeł przechowuje wartość liczbową
// czy jest wskaźnikiem na inny stos.
typedef enum {
    TYPE_VALUE,
    TYPE_STACK
} element_type_t;

// Typ, który określa w jakim stanie jest stos podczas wykowywania
// algorytmu usuwania stosu.
typedef enum {
    WHITE,
    GREY,
    BLACK
} color_t;

// Unia, która określa wartość liczbową oraz wskaźnik na stos
// w zależności od typu danych.
typedef union {
    uint64_t value;
    struct rstack* stack;
} element_data_t;

// Struktura, która reprezentuje węzeł znajdujący się na stosie.
typedef struct rstack_node {
    element_type_t type;
    element_data_t data;
    struct rstack_node* next;
} rstack_node_t;

// Struktura, która reprezentuje stos.
typedef struct rstack {
    size_t ref_count;
    rstack_node_t* top;
    color_t color;
} rstack_t;


// Funkcja tworzy nowy pusty stos.
// Wynikiem funkcji jest wskaźnik na strukturę reprezentującą stos lub nullptr,
// jeśli wystąpił błąd przydzielania pamięci.
rstack_t* rstack_new() {
    rstack_t* new_stack = malloc(sizeof(rstack_t));
    if (new_stack == nullptr) {
        errno = ENOMEM;
        return nullptr;
    }

    new_stack->ref_count = 1;
    new_stack->top = nullptr;
    new_stack->color = BLACK;
    return new_stack;
}

// Funkcja tworzy nowy węzeł przechowujący wartość liczbową,
// który umieszczany jest na szczycie wskazanego stosu.
// Wynikiem funkcji jest 1, jeśli operacja zakończyła się sukcesem,
// a w przeciwnym wypadku -1.
int rstack_push_value(rstack_t *rs, uint64_t value) {
    if (rs == nullptr) {
        errno = EINVAL;
        return -1;
    }

    rstack_node_t* new_node = malloc(sizeof(rstack_node_t));
    if (new_node == nullptr) {
        errno = ENOMEM;
        return -1;
    }
    
    new_node->type = TYPE_VALUE;
    new_node->data.value = value;
    new_node->next = rs->top;
    rs->top = new_node;
    return 0;
}

// Funkcja tworzy nowy węzeł przechowujący wskaźnik na stos,
// który umieszczany jest na szczycie wskazanego stosu.
// Wynikiem funkcji jest 1, jeśli operacja zakończyła się sukcesem,
// a w przeciwnym wypadku -1.
// rs1 - wskaźnik na strukturę reprezentującą stos, na który odkładany jest stos
// rs2 – wskaźnik na strukturę reprezentującą stos, który jest odkładany
int rstack_push_rstack(rstack_t *rs1, rstack_t *rs2) {
    if (rs1 == nullptr || rs2 == nullptr) {
        errno = EINVAL;
        return -1;
    }

    rstack_node_t* new_node = malloc(sizeof(rstack_node_t));
    if (new_node == nullptr) {
        errno = ENOMEM;
        return -1;
    }

    new_node->type = TYPE_STACK;
    new_node->data.stack = rs2;
    new_node->next = rs1->top;
    rs1->top = new_node;
    (rs2->ref_count)++;
    return 0;
}

// Funkcja przeszukuje graf stosów w celu znalezienia wartości
// liczbowej leżącej najbliżej szczytu danego stosu.
// Wynikiem funkcji jest {true, value}, jeśli znaleziona została
// wartość liczbowa, a w przeciwnym wypadku wynikiem jest {false, 0}.
// Modyfikuje kolory stosów, aby uniknąć nieskończonej rekurencji
// w przypadku wystąpienia cykli.
result_t rstack_search_value(rstack_t *rs) {
    if (rs == nullptr) {
        result_t result = {false, 0};
        return result;
    }
    
    // Kolor szary informuje o tym, że stos był już odwiedzony,
    // więc oznacza to wejście w cykl.
    // Kolor biały informuje o tym, że stos był już wcześniej przeszukany
    // i nie zawiera żadnych wartości liczbowych.
    if (rs->color == GREY || rs->color == WHITE) {
        result_t result = {false, 0};
        return result;
    }

    rs->color = GREY;
    result_t result = {false, 0};
    rstack_node_t* current_node = rs->top;
    bool found = false;

    while (found == false && current_node != nullptr) {
        if (current_node->type == TYPE_VALUE) {
            result = (result_t){true, current_node->data.value};
            found = true;
        } 
        else {
            // Szukanie rekurencyjne wartości liczbowej wewnątrz stosu, 
            // którego wskaźnik znajduje się na stosie rs.
            result_t value_search = rstack_search_value(current_node->data.stack);
            if (value_search.flag == true) {
                result = value_search;
                found = true;
            }
        }
        current_node = current_node->next;
    }

    // Oznacza to, że wewnątrz stosu rs nie ma żadnej wartości liczbowej.
    if (result.flag == false) {
        rs->color = WHITE; 
    }
    
    return result;
}

// Funkcja przechodzi rekurencyjnie przez elementy stosu rs
// i przywraca domyślony kolor stosom, które były
// zmodyfikowane podczas operacji szukania wartości liczbowej.
void reset_colors(rstack_t *rs) {
    // Jeśli stos jest czarny to znaczy, że został już przywrócony jego kolor.
    if (rs == nullptr || rs->color == BLACK) {
        return;
    }

    rs->color = BLACK;
    rstack_node_t* current_node = rs->top;

    while (current_node != nullptr) {
        if (current_node->type == TYPE_STACK) {
            reset_colors(current_node->data.stack);
        }
        current_node = current_node->next;
    }
}

// Funkcja znajduje rekurencyjnie liczbę, która jest najbliżej szczytu stosu.
result_t rstack_front(rstack_t *rs) {
    result_t result = rstack_search_value(rs);
    reset_colors(rs);
    return result;
}

// Funkcja sprawdza rekurencyjnie, czy stos zawiera liczbę.
// Wynikiem funkcji jest 1, jeśli stos nie zawiera liczby,
// a w przeciwnym wypadku 0.
bool rstack_empty(rstack_t *rs) {
    result_t result = rstack_search_value(rs);
    reset_colors(rs);
    return (result.flag == 0);
}

// Funkcja wprowadza próbne usuwanie stosów poprzez odejmowanie liczników
// referencji stosom, do których możliwe jest dostanie się ze stosu rs.
// Celem jest sprawdzenie czy analizowany graf stosów jest w postaci izolowanej.
void mark_grey(rstack_t* rs) {
    if (rs->color == GREY) {
        return;
    }
    
    rs->color = GREY;
    rstack_node_t* current_node = rs->top;

    while (current_node != nullptr) {
        if (current_node->type == TYPE_STACK) {
            rstack_t* stack = current_node->data.stack;
            (stack->ref_count)--;
            mark_grey(stack);
        }
        current_node = current_node->next;
    }
}

// Funkcja przywraca pierwotny stan liczników referencji oraz
// czarny kolor stosom, które, po próbnym usuwaniu, okazało się, że nie są
// częścią izolowanego grafu stosów.
void revert_to_black(rstack_t* rs) {
    if (rs->color == BLACK) {
        return;
    }

    rs->color = BLACK;
    rstack_node_t* current_node = rs->top;

    while (current_node != nullptr) {
        if (current_node->type == TYPE_STACK) {
            rstack_t* element_stack = current_node->data.stack;
            // Przywraca wartość licznika referencji.
            (element_stack->ref_count)++;
            revert_to_black(element_stack);
        }
        current_node = current_node->next;
    }
}

// Funkcja sprawdza, czy po próbnym usuwaniu stosów badany stos nadal
// posiada referencje poza przeszukiwanym grafem stosów.
// Jeśli licznik wyniósł zero, to stos należy do izolowanego grafu stosów,
// więc zostaje oznaczony kolorem białym w celu usunięcia.
// W przeciwnym wypadku przywracany jest stosowi kolor czarny.
void search_for_isolated_cycles(rstack_t* rs) {
    if (rs->color != GREY) {
        return;
    }

    if (rs->ref_count > 0) {
        revert_to_black(rs);
    }

    else {
        rs->color = WHITE;
        rstack_node_t* current_node = rs->top;

        while (current_node != nullptr) {
            if (current_node->type == TYPE_STACK) {
                search_for_isolated_cycles(current_node->data.stack);
            }
            current_node = current_node->next;
        }
    }
}

// Funkcja zwalnia pamięć elementów stosu należącego do izolowanego grafu stosów
// i tworzy listę stosów do usunięcia.
// W celu uniknięcia nieskończonej rekurencji kolor odwiedzonych
// stosów jest zmieniany z białego na czarny.
void delete_white_nodes(rstack_t* rs, rstack_t** list_stack_to_delete) {
    // Stosy nieoznaczone kolorem białym nie są usuwane.
    if (rs == nullptr || rs->color != WHITE) {
        return;
    }

    // Zabezpiecza przed nieskończoną rekurencją.
    rs->color = BLACK;
    rstack_node_t* current_node = rs->top;

    // Węzły zostaną usunięte, więc wskaźnik top jest wykorzystany do
    // utworzenia listy stosów, które zostaną usunięte
    // wewnątrz funkcji clear_white_stacks.
    // Proces ten zapobiega alokacji nowej pamięci.
    rs->top = (rstack_node_t*) *list_stack_to_delete;
    *list_stack_to_delete = rs;

    while (current_node != nullptr) {
        rstack_node_t* node_to_delete = current_node;
        current_node = current_node->next;

        if (node_to_delete->type == TYPE_STACK) {
            delete_white_nodes(node_to_delete->data.stack, list_stack_to_delete);
        }

        free(node_to_delete);
    }
}

// Funkcja usuwa izolowany graf stosów.
// Najpierw niszczy węzły stosów, a następnie usuwa stosy znajdujące
// się na utworzonej liście stosów.
void delete_white_stacks(rstack_t* rs) {
    rstack_t* list_stack_to_delete = nullptr;
    delete_white_nodes(rs, &list_stack_to_delete);
    rstack_t* current_stack = list_stack_to_delete;

    while (current_stack != nullptr) {
        rstack_t* next_stack = (rstack_t*) current_stack->top;
        free(current_stack);
        current_stack = next_stack;
    }
}

// Funkcja sprawdza, czy istnieje element na stosie rs,
// który jest wskaźnikiem na inny stos.
bool search_for_stack(rstack_t* rs) {
    rstack_node_t* current_node = rs->top;
    while (current_node != nullptr) {
        if (current_node->type == TYPE_STACK) {
            return true;
        }
        current_node = current_node->next;
    }
    return false;
}

// Funkcja kasuje stos.
void rstack_delete(rstack_t *rs) {
    if (rs == nullptr) {
        return;
    }
    
    (rs->ref_count)--;
    
    // Brak referencji na usuwany stos
    if (rs->ref_count == 0) {
        while (rs->top != nullptr) {
            rstack_pop(rs); 
        }
        free(rs);
    }
    else {
        // Szukanie izolowanego grafu stosów uruchamiane jest, gdy dany stos
        // przechowuje wewnątrz inne stosy.
        if (search_for_stack(rs) == true) {

            //Algorytm wykrywania i usuwania izolowanych grafów:
            // Odejmuje licznik referencji w celu znalezienia izolowanych grafów.
            mark_grey(rs);
            // Sprawdza czy istnieje stos nienależący do rozpatrywanego grafu,
            // który przechowuje wskaźnik na dany stos.
            search_for_isolated_cycles(rs);
            // Usuwa stosy należące do izolowanego grafu stosów.
            delete_white_stacks(rs);
        }
    }
}

// Funkcja zdejmuje nierekurencyjnie wierzchołek stosu.
void rstack_pop(rstack_t *rs) {
    if (rs != nullptr) {
        rstack_node_t* node = rs->top;

        if (node != nullptr) {
            rs->top = node->next;
            // Jeśli wierzchołkiem stosu był wskaźnik na stos, to należy
            // wywołać usuwanie stosu dla tego elementu.
            if (node->type == TYPE_STACK) {
                rstack_delete(node->data.stack);
            }
            free(node);
        }
    }
}

// Deklaracja w przód funkcji niezawartej w pliku nagłówkowym rstack.h
int rstack_numbers_cycles_recursive(rstack_t *rs, FILE* file);

// Funkcja zapisuje rekurencyjnie do pliku elementy od
// dołu stosu do jego wierzchołka.
// Wynikiem jest 0, jeśli operacja zakończyła się sukcesem, 1 jeśli
// przy zapisywaniu natrafiono na cykl, a w przeciwnym wypadku -1.
int write_nodes_reversed_order(rstack_node_t* node, FILE* file) {
    if (node == nullptr) {
        return 0;
    }
    
    int result = write_nodes_reversed_order(node->next, file);
    if (result != 0) {
       return result;
    }
    
    // Element będący wartością liczbową wpisuje do pliku.
    if (node->type == TYPE_VALUE) {
        // Błąd wczytywania do pliku
        if (fprintf(file, "%" PRIu64 "\n", node->data.value) < 0) {
            return -1;
        }
    }

    // Jeśli elementem na stosie jest wskaźnik do stosu, to
    // sprawdza czy nie generuje on cyklu.
    else {
        result = rstack_numbers_cycles_recursive(node->data.stack, file);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

// Funkcja sprawdza czy stos, którego elementy liczbowe mają
// być wczytane do pliku tworzy cykl.
int rstack_numbers_cycles_recursive(rstack_t *rs, FILE* file) {
    if (rs == nullptr) {
        return -1;
    }

    // Natrafienie na stos o kolorze szarym oznacza, że funkcja weszła w cykl,
    // więc wypisywanie zostaje przerwane.
    if (rs->color == GREY) {
         return 1;
    }

    rs->color = GREY;
    int result = write_nodes_reversed_order(rs->top, file);

    rs->color = BLACK;
    
    return result;
}

// Funkcja zapisuje do pliku o ścieżce path liczby odłożone na stosie rs.
// Obsługiwane są błędy wczytania pliku, stosu i danych do pliku.
// Wynikiem funkcji jest 0, jeśli operacja zakończyła się sukcesem,
// a w przeciwnym wypadku -1. 
int rstack_write(char const *path, rstack_t *rs) {
    if (rs == nullptr || path == nullptr) {
        errno = EINVAL;
        return -1;
    }

    // Jeśli stos nie zawiera wartości liczbowych, to zwracany jest pusty
    // plik lub informacja o błędzie.
    if (rstack_empty(rs)) {
        FILE* file = fopen(path, "w");
        if (file != nullptr) {
            fclose(file);
            return 0;
        }
        return -1;
    }

    FILE* file = fopen(path, "w");
    if (file == nullptr) {
        return -1;
    }

    int wynik = rstack_numbers_cycles_recursive(rs, file);
    fclose(file);
    
    if(wynik == -1) {
        return -1;
    }
    return 0;
}

// Funkcja czyta pojedynczą liczbę typu uint64_t z pliku file.
// Wynikiem funkcji jest 1, jeśli operacja zakończyła się sukcesem, 0 jeśli
// jest koniec pliku, a w przeciwnym wypadku -1.
// W parametrze *value_file przekazywana jest wartość odczytanej liczby.
int read_value_file(FILE *file, uint64_t *value_file) {
    int c;

    // Pomija białe znaki z pliku.
    do {
        c = fgetc(file);
    } while (isspace(c));

    // Dotarcie do końca pliku
    if (c == EOF) {
        return 0;
    }

    // Jeśli znakiem nie jest cyfra, to zwraca informacje o błędzie.
     if (!isdigit(c)) {
        return -1;
    }

    uint64_t value = 0;
    int number_length = 1;
    bool first_digit_zero = (c == '0');

    // Tworzy liczbę z cyfr znajdujących się w pliku.
    while (isdigit(c)) {
        // Jeśli liczba nie jest zerem, to nie może zaczynać się zerem.
        if (number_length > 1 && first_digit_zero == true) {
            return -1;
        }
        // Sprawdza czy po dodaniu nowej cyfry wartość liczby nie wyjdzie
        // poza zakres typu uint64.
        if ((value > UINT64_MAX / 10) || 
            (value == UINT64_MAX / 10 && 
            (c - '0') > (int)(UINT64_MAX % 10))) {
            return -1;
        }

        value = value * 10 + (c - '0');
        number_length++;
        c = fgetc(file);
    }

    // Jeśli wczytana wartość nie jest białym znakiem lub końcem pliku
    // to zwraca informacje o błędzie.
    if (c != EOF && isspace(c) == false) {
        return -1;
    }

    *value_file = value;
    return 1;
}

// Funkcja tworzy nowy stos, na którym odkłada
// wczytane liczby z pliku o ścieżce path.
rstack_t* rstack_read(char const *path) {
    if (path == nullptr) {
        errno = EINVAL;
        return nullptr;
    }

    FILE* file = fopen(path, "r");
    if (file == nullptr) {
        return nullptr;
    }

    rstack_t* rs = rstack_new();

    // Obsługuje błąd alokacji pamięci dla stosu.
    if (rs == nullptr) {
        // Zapisuje aktualną wartość errno, aby nie została
        // zmieniona podczas wywołania zamykania pliku.
        int saved_errno = errno;
        fclose(file);
        errno = saved_errno;
        return nullptr;
    }

    uint64_t value;
    int result_of_read;
    bool fail = false;

    // Wczytuje wartości liczbowe z pliku.
    while (fail == false && (result_of_read = read_value_file(file, &value)) == 1) {
        if (rstack_push_value(rs, value) != 0) {
            result_of_read = -1;
            fail = true;
        }
    }

    // Usuwa stos, którego nie udało się utworzyć do końca.
    if (result_of_read == -1 || ferror(file)) {
        int marked_errno = errno;
        if(marked_errno == 0) {
            errno = EINVAL;
        }

        rstack_delete(rs);
        fclose(file);

        // Przywraca właściwy powód błędu.
        errno = marked_errno;
        return nullptr;
    }

    fclose(file);
    return rs;
}