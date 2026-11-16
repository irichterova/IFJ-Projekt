/**
 * Project name: Compiler for IFJ26
 * @author Vojtěch Ziaťko, xziatkv00
 * @file expression.c
 */

#include <stdlib.h>
#include "expression.h"
#include "error.h"

#define PSA_TABLE_SIZE 19
#define INITIAL_CAPACITY 16

// Symbols defining the table, order must match rows/columns of precedence_table
typedef enum {
    PSA_UNARY_MINUS, // ~
    PSA_MUL,         // *
    PSA_DIV,         // /
    PSA_ADD,         // +
    PSA_SUB,         // -
    PSA_LESS,        // <
    PSA_LESS_EQ,     // <=
    PSA_GREATER,     // >
    PSA_GREATER_EQ,  // >=
    PSA_EQUAL,       // ==
    PSA_NOT_EQUAL,   // !=
    PSA_ID,          // i
    PSA_LEFT_P,      // (
    PSA_RIGHT_P,     // )
    PSA_START_END,   // $
    PSA_CAT,         // ++ [LISTS]
    PSA_NOT,         // !  [BOOLTHEN]
    PSA_AND,         // && [BOOLTHEN]
    PSA_OR,          // || [BOOLTHEN]
    // Below are stack only symbols, not in the table
    PSA_E,           // Expression E
    PSA_STOP,        // Marker <
} psa_symbol;

// What can be inside the table
typedef enum {
    PSA_REL_SHIFT,  // <
    PSA_REL_PUSH,   // =
    PSA_REL_REDUCE, // >
    PSA_REL_ERROR,  // Empty
} psa_relation;

static const psa_relation precedence_table[PSA_TABLE_SIZE][PSA_TABLE_SIZE] = {
    //          ~                *               /               +               -               <               <=             >               >=                ==              !=            i               (               )                $               ++              !           &&              ||
    /* ~ */ {PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* * */ {PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* / */ {PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* + */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* - */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* < */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* <= */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* > */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* >= */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* == */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* != */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* i */ {PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* ( */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_PUSH, PSA_REL_ERROR, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT},
    /* ) */ {PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_ERROR, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* $ */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_ERROR, PSA_REL_ERROR, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT},
    /* ++ */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* ! */ {PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* && */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE},
    /* || */ {PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE, PSA_REL_REDUCE, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_SHIFT, PSA_REL_REDUCE},
};

/**
 * @brief Transforms a token into a corresponding symbol
 * @param token Token from scanner
 * @param prev What symbol came before this one
 * @param depth Number of currently open parentheses
 * @return Symbol representing the token
 */
static psa_symbol token2symbol (token_t token, psa_symbol prev, int depth) {
    // EOL outside parentheses means end of expression
    if (token.after_eol && depth == 0) {
        return PSA_START_END;
    }

    switch (token.type) {
        // All of these result in i
        case TOKEN_IDENTIFIER:
        case TOKEN_INT_LITERAL:
        case TOKEN_DBL_LITERAL:
        case TOKEN_STR_LITERAL:
        case TOKEN_TRUE_LITERAL:
        case TOKEN_FALSE_LITERAL:
            return PSA_ID;

        case TOKEN_OP_ADD:
            return PSA_ADD;
        case TOKEN_OP_MUL:
            return PSA_MUL;
        case TOKEN_OP_DIV:
            return PSA_DIV;
        case TOKEN_OP_LT:
            return PSA_LESS;
        case TOKEN_OP_LE:
            return PSA_LESS_EQ;
        case TOKEN_OP_GT:
            return PSA_GREATER;
        case TOKEN_OP_GE:
            return PSA_GREATER_EQ;
        case TOKEN_OP_EQ:
            return PSA_EQUAL;
        case TOKEN_OP_NEQ:
            return PSA_NOT_EQUAL;

        case TOKEN_OP_NOT:
            return PSA_NOT;
        case TOKEN_OP_AND:
            return PSA_AND;
        case TOKEN_OP_OR:
            return PSA_OR;

        case TOKEN_L_PARENTH:
            return PSA_LEFT_P;
        case TOKEN_R_PARENTH:
            if (depth == 0) {
                return PSA_START_END; // $
            }
            return PSA_RIGHT_P;

        case TOKEN_OP_SUB:
            // Scanner doesn't distinguish unary and binary minus
            // So we do it ourselves
            if (prev == PSA_ID || prev == PSA_RIGHT_P) {
                return PSA_SUB;
            }
            return PSA_UNARY_MINUS;

        default:
            // Any other token (then, do, ',', EOF, etc.) ends the expression
            return PSA_START_END;
    }
}

// Stack code below

typedef struct psa_stack_item_t {
    psa_symbol item_symbol; // Terminal, PSA_E or PSA_STOP
    token_t item_token;     // For terminals only
    expr_struct* subtree;   // Pointer to subtree created by reduction
    int line;               // Source line: terminal = line of its token, E = line set in reduction
} Stack_Item;

typedef struct psa_stack_t {
    Stack_Item* stack_items; // Array of malloc'd items
    int top;                 // Item count, top item is at top-1
    int capacity;            // Maximum capacity of the stack
} PSA_Stack;

/**
 * @brief Create a new stack
 * @param stack Pointer to stack variable
 */
static void init_stack (PSA_Stack *stack) {
    // Init a new stack
    stack->stack_items = malloc(sizeof(Stack_Item) * INITIAL_CAPACITY);
    // Failed malloc
    if (stack->stack_items == NULL) {
        error_exit(ERR_INTERNAL, 0, "Failed to malloc a new PSA Stack in init_stack!");
    }
    stack->capacity = INITIAL_CAPACITY;
    stack->top = 0;
}

/**
 * @brief Push an item to the top of the stack
 * @param stack Stack to push into
 * @param item Item to put at the top
 */
static void push_item (PSA_Stack *stack, Stack_Item item) {
    // Add an item, expand if full
    if (stack->top >= stack->capacity) {
        stack->capacity *= 2;

        Stack_Item *temp_stack_items = realloc(stack->stack_items, sizeof(Stack_Item) * stack->capacity);
        // Failed realloc
        if (temp_stack_items == NULL) {
            free(stack->stack_items);
            error_exit(ERR_INTERNAL, 0, "Failed to realloc a new PSA Stack in push_item!");
        }
        stack->stack_items = temp_stack_items;
    }
    stack->stack_items[stack->top] = item;
    stack->top += 1;
}

/**
 * @brief Pop an item from the stack
 * @param stack Pointer to stack variable
 * @return The removed (formerly the top) item
 */
static Stack_Item pop_item (PSA_Stack *stack) {
    if (stack->top == 0) {
        error_exit(ERR_INTERNAL, 0, "Tried to pop an empty stack!");
    }
    stack->top--;
    return stack->stack_items[stack->top];
}

/**
 * @brief Read the top item on stack
 * @param stack Pointer to stack variable
 * @return Item at the top
 */
static Stack_Item top_item (PSA_Stack *stack) {
    if (stack->top == 0) {
        error_exit(ERR_INTERNAL, 0, "Tried to read top of an empty stack!");
    }
    return stack->stack_items[stack->top - 1];
}

/**
 * @brief Find the top-most terminal
 * @param stack Pointer to stack variable
 * @return Position of the top-most terminal
 */
static int top_terminal (PSA_Stack *stack) {
    // top - 1 is the top-most stack item
    for (int i = stack->top - 1; i >= 0; i--) {
        if (stack->stack_items[i].item_symbol != PSA_E && stack->stack_items[i].item_symbol != PSA_STOP) {
            return i;
        }
    }
    // None was found
    error_exit(ERR_INTERNAL, 0, "Couldn't find a terminal symbol in stack!");
    return -1;
}

/**
 * @brief Insert stop marker < after top-most terminal (start of handle)
 * @param stack Pointer to stack variable
 */
static void insert_stop (PSA_Stack *stack) {
    if (stack->top == 0) {
        error_exit(ERR_INTERNAL, 0, "Tried to insert stop on an empty stack in insert_stop!");
    }

    // Check for space and increase capacity if we need to
    if (stack->top >= stack->capacity) {
        stack->capacity *= 2;

        Stack_Item *temp_stack_items = realloc(stack->stack_items, sizeof(Stack_Item) * stack->capacity);
        // Failed realloc
        if (temp_stack_items == NULL) {
            free(stack->stack_items);
            error_exit(ERR_INTERNAL, 0, "Failed to realloc a new PSA Stack in insert_stop!");
        }
        stack->stack_items = temp_stack_items;
    }
    // Go over items from the top, if E or < skip, if not insert PSA_STOP
    for (int i = stack->top - 1; i >= 0; i--) {
        if (stack->stack_items[i].item_symbol == PSA_E || stack->stack_items[i].item_symbol == PSA_STOP) {
            stack->stack_items[i + 1] = stack->stack_items[i];
        } else {
            stack->stack_items[i + 1] = (Stack_Item){ .item_symbol = PSA_STOP, .subtree = NULL };
            break;
        }
        if (i == 0) {
            error_exit(ERR_INTERNAL, 0, "Failed to insert stop in insert_stop!");
        }
    }
    stack->top++;
}

/**
 * @brief Frees the stack, not the subtrees
 * @param stack Pointer to stack variable
 */
static void free_stack (PSA_Stack *stack) {
    free(stack->stack_items);
    stack->stack_items = NULL;
    stack->capacity = 0;
    stack->top = 0;
}

// Stack code above

/**
 * @brief Converts a binary operator PSA symbol to operator_type from ast.h
 * @param symbol Symbol to convert
 * @param line Line for the error message
 * @return Corresponding operator from ast.h
 */
static operator_type symbol2operator(psa_symbol symbol, int line) {
    switch (symbol) {
    case PSA_AND:
        return OP_AND;
    case PSA_OR:
        return OP_OR;
    case PSA_ADD:
        return OP_PLUS;
    case PSA_SUB:
        return OP_BIN_MINUS;
    case PSA_MUL:
        return OP_MULTIPLICATION;
    case PSA_DIV:
        return OP_DIVISION;
    case PSA_LESS:
        return OP_LESS;
    case PSA_LESS_EQ:
        return OP_EQUAL_LESS;
    case PSA_GREATER:
        return OP_GREATER;
    case PSA_GREATER_EQ:
        return OP_EQUAL_GREATER;
    case PSA_EQUAL:
        return OP_EQUAL;
    case PSA_NOT_EQUAL:
        return OP_NOTEQUAL;
    default:
        // Default should only run when the middle op isn't a bin op like E ( E
        error_exit(ERR_SYN, line, "Could not transform symbol to operator!");
        return OP_PLUS; // Never reached, only for compiler
    }
}

/**
 * @brief Reduces handle into a subtree of a single new E and pushes it
 * @param stack Pointer to stack variable
 */
static void reduce(PSA_Stack *stack) {
    int count = 0;
    Stack_Item handle[3] = {0}; // We can reduce a maximum of three (3) items, otherwise error

    Stack_Item item_top = top_item(stack);
    while (item_top.item_symbol != PSA_STOP) {
        if (count == 3) {
            error_exit(ERR_SYN, handle[0].line, "Tried to reduce too many items!");
        }
        handle[count] = pop_item(stack);
        count++;
        // Update top item
        item_top = top_item(stack);
    }
    // Now it's PSA_STOP < at the top, so we pop it
    pop_item(stack);

    // E -> ( E ) Nothing new, reuse
    if (count == 3 && handle[2].item_symbol == PSA_LEFT_P && handle[1].item_symbol == PSA_E && handle[0].item_symbol == PSA_RIGHT_P) {
        Stack_Item new_item = {.item_symbol = PSA_E, .line = handle[2].line, .subtree = handle[1].subtree};
        push_item(stack, new_item);
        return;
    }

    // This will be our new subtree
    expr_struct *node = malloc(sizeof(expr_struct));
    if (node == NULL) {
        error_exit(ERR_INTERNAL, handle[0].line, "Failed to allocate memory for a new AST node while reducing!");
    }

    int line = 0;

    // E -> i
    if (count == 1 && handle[0].item_symbol == PSA_ID) {
        line = handle[0].line;

        switch (handle[0].item_token.type) {
        case TOKEN_IDENTIFIER:
            node->node_type = EXPR_VARIABLE;
            node->data.variable.id_name = handle[0].item_token.data.id;
            node->result_type = TYPE_UNKNOWN; // TODO: Type check done later in a shared type-check function
            break;
        
        case TOKEN_INT_LITERAL:
            node->node_type = EXPR_LIT_INT;
            node->data.lit_int = handle[0].item_token.data.int_value;
            node->result_type = TYPE_INT;
            break;

        case TOKEN_DBL_LITERAL:
            node->node_type = EXPR_LIT_DOUBLE;
            node->data.lit_double = handle[0].item_token.data.dbl_value;
            node->result_type = TYPE_DOUBLE;
            break;

        case TOKEN_STR_LITERAL:
            node->node_type = EXPR_LIT_STRING;
            node->data.lit_string = handle[0].item_token.data.str_value;
            node->result_type = TYPE_STRING;
            break;
        
        case TOKEN_TRUE_LITERAL:
            node->node_type = EXPR_LIT_TRUE;
            node->result_type = TYPE_BOOL;
            break;

        case TOKEN_FALSE_LITERAL:
            node->node_type = EXPR_LIT_FALSE;
            node->result_type = TYPE_BOOL;
            break;

        default:
            error_exit(ERR_SYN, handle[0].line, "Incorrect E -> i expression grammar!");
        }
    }

    // E -> ~E
    else if (count == 2 && handle[0].item_symbol == PSA_E && handle[1].item_symbol == PSA_UNARY_MINUS) {
        line = handle[1].line;

        // TODO: Type check done later in a shared type-check function (-string not allowed)
        node->node_type = EXPR_UNA_OP;
        node->data.unary_op.op_type = OP_UNA_MINUS;
        node->data.unary_op.operand = handle[0].subtree;
        node->result_type = handle[0].subtree->result_type;
    }

    // E -> !E
    else if (count == 2 && handle[0].item_symbol == PSA_E && handle[1].item_symbol == PSA_NOT) {
        line = handle[1].line;

        // TODO: Type check done later in a shared type-check function (!string or !int/double not allowed)
        node->node_type = EXPR_UNA_OP;
        node->data.unary_op.op_type = OP_NOT;
        node->data.unary_op.operand = handle[0].subtree;
        node->result_type = TYPE_BOOL;
    }

    // E -> E op E
    else if (count == 3 && handle[2].item_symbol == PSA_E && handle[0].item_symbol == PSA_E) {
        line = handle[1].line;

        node->node_type = EXPR_BIN_OP;
        node->data.binary_op.op_type = symbol2operator(handle[1].item_symbol, handle[1].line);
        node->data.binary_op.left_operand = handle[2].subtree;
        node->data.binary_op.right_operand = handle[0].subtree;
        node->result_type = TYPE_UNKNOWN; // TODO: Type check done later in a shared type-check function
    }

    // E -> () special Unit rule
    else if (count == 2 && handle[1].item_symbol == PSA_LEFT_P && handle[0].item_symbol == PSA_RIGHT_P) {
        line = handle[1].line;
        node->node_type = EXPR_LIT_UNIT;
        node->result_type = TYPE_UNIT;
    }
    
    else {
        error_exit(ERR_SYN, handle[0].line, "Handle doesn't correspond to any grammar rule!");
    }

    node->line = line;

    Stack_Item new_item = {.item_symbol = PSA_E, .line = line, .subtree = node};

    // Push a new item with a new subtree to stack
    push_item(stack, new_item);
}

/**
 * @brief The main loop of precedence analysis, called by the two main functions based on token count
 * @param first Starting token of expression
 * @param second Second token of expression, already loaded
 * @param has_second True/false if we load second token that we were given instead of calling a new token directly
 * @param next Pointer to next token, we store it there
 * @return AST node
 */
static expr_struct *run_psa(token_t first, token_t second, int has_second, token_t *next) {
    // First we init stack and push first $

    PSA_Stack stack;
    init_stack(&stack);

    Stack_Item start_item = {.item_symbol = PSA_START_END};
    push_item(&stack, start_item);

    token_t token = first;              // Current working token
    token.after_eol = false;            // Newline from tokens from parser is ignored
    psa_symbol prev = PSA_START_END;    // Previous symbol
    int depth = 0;                      // Tracks the number of open parentheses
    int line = get_line_number();

    psa_symbol sym = token2symbol(token, prev, depth);
    int second_token_pending = has_second; // Set to 0 after used

    // Main loop
    while(1) {
        int t = top_terminal(&stack);
        psa_symbol top_sym = stack.stack_items[t].item_symbol;

        // End condition [$, $]
        if (top_sym == PSA_START_END && sym == PSA_START_END) {
            break;
        }

        Stack_Item item;
        switch (precedence_table[top_sym][sym]) {
        case PSA_REL_SHIFT:
            insert_stop(&stack);
            /* Fallthrough */
        case PSA_REL_PUSH:
            item = (Stack_Item){.item_symbol = sym, .item_token = token, .line = line};
            push_item(&stack, item);

            // Update parentheses depth
            if (sym == PSA_LEFT_P) {
                depth++;
            } else if (sym == PSA_RIGHT_P) {
                depth--;
            }
            // Save current symbol and get a new token and convert it
            prev = sym;
            if (second_token_pending) {
                second_token_pending = 0;
                token = second;
                token.after_eol = false;
            } else {
                token = get_token(EOL_OPTIONAL);
            }
            line = get_line_number();
            sym = token2symbol(token, prev, depth);
            break;

        case PSA_REL_REDUCE:
            reduce(&stack);
            break;
        case PSA_REL_ERROR:
            error_exit(ERR_SYN, line, "Incorrect expression syntax!");
            break;
        
        default:
            error_exit(ERR_SYN, line, "Relation somehow not in precedence table?");
            break;
        }
    }

    // Save the tree
    Stack_Item final_item = pop_item(&stack);
    if (final_item.item_symbol != PSA_E) {
        error_exit(ERR_SYN, line, "Incorrect expression structure!");
    }

    expr_struct* root = final_item.subtree;

    *next = token;
    free_stack(&stack);
    return root;
}

/**
 * @brief Parses an expression (expr OR expr_noid), creates its tree, stores following token
 * @param first First token
 * @param next Where to store the following token
 * @return Pointer to the root of the expression tree
 */
expr_struct *parse_expr(token_t first, token_t *next) {
    // When we only know one token, disable the second
    return run_psa(first, first, 0, next);
}

/**
 * @brief Parses an expression (expr_rest), creates its tree, stores following token
 * @param first First token
 * @param second Second token
 * @param next Where to store the following token
 * @return Pointer to the root of the expression tree
 */
expr_struct *parse_expr_rest(token_t first, token_t second, token_t *next) {
    // When we get a second token as well, use it instead of asking for a new one
    return run_psa(first, second, 1, next);
}
