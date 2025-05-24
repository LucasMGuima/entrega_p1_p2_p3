#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // Para isalpha, isdigit, isspace, isalnum

// --- 1. Tipos de Tokens ---
typedef enum {
    // Palavras-chave
    TOKEN_FUNC, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_BREAK,
    TOKEN_INT, TOKEN_CHAR,

    // Operadores e Símbolos
    TOKEN_ASSIGN, TOKEN_SEMICOLON, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_COMMA,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,
    TOKEN_AND, TOKEN_OR,
    TOKEN_NE, TOKEN_GT, TOKEN_LT, TOKEN_EQ, TOKEN_LE, TOKEN_GE,

    // Literais
    TOKEN_NUMBER, TOKEN_STRING_LITERAL,

    // Identificador
    TOKEN_IDENTIFIER,

    // Outros
    TOKEN_EOF, TOKEN_ERROR
} TokenType;

// --- 2. Estrutura do Token ---
typedef struct {
    TokenType type;
    char* value; // O lexema (string) do token
    int line;
    int column;
} Token;

// --- 3. Estrutura do Lexer ---
// O lexer mantém o estado de onde ele está lendo no código fonte
typedef struct {
    const char* source_code; // Ponteiro para o código fonte
    int current_pos;         // Posição atual no código
    char current_char;       // Caractere atual sendo processado
    int line;                // Número da linha atual
    int column;              // Coluna atual na linha
    int source_len;          // Comprimento total do código fonte
} Lexer;

// --- Funções Auxiliares do Lexer ---

// Avança para o próximo caractere
void advance(Lexer* lexer) {
    lexer->current_pos++;
    lexer->column++;
    if (lexer->current_pos < lexer->source_len) {
        lexer->current_char = lexer->source_code[lexer->current_pos];
    } else {
        lexer->current_char = '\0'; // Marcador de fim de arquivo
    }
}

// Retorna o próximo caractere sem avançar a posição
char peek(Lexer* lexer, int offset) {
    int peek_pos = lexer->current_pos + offset;
    if (peek_pos < lexer->source_len) {
        return lexer->source_code[peek_pos];
    }
    return '\0';
}

// Pula espaços em branco, tabs e quebras de linha
void skip_whitespace(Lexer* lexer) {
    while (lexer->current_char != '\0' && isspace(lexer->current_char)) {
        if (lexer->current_char == '\n') {
            lexer->line++;
            lexer->column = 0; // Coluna 0 para a nova linha
        }
        advance(lexer);
    }
}

// Cria e retorna um novo token alocado dinamicamente
Token* create_token(TokenType type, const char* value, int line, int column) {
    Token* token = (Token*)malloc(sizeof(Token));
    if (token == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria para token.\n");
        exit(EXIT_FAILURE);
    }
    token->type = type;
    token->value = (value != NULL) ? strdup(value) : NULL; // Duplica a string
    token->line = line;
    token->column = column;
    return token;
}

// Libera a memória de um token
void free_token(Token* token) {
    if (token != NULL) {
        free(token->value); // Libera a string alocada por strdup
        free(token);
    }
}

// --- Funções para Reconhecer Tipos de Tokens Específicos ---

Token* read_number(Lexer* lexer) {
    int start_column = lexer->column;
    int start_pos = lexer->current_pos;
    while (lexer->current_char != '\0' && isdigit(lexer->current_char)) {
        advance(lexer);
    }
    int length = lexer->current_pos - start_pos;
    char* num_str = (char*)malloc(length + 1);
    if (num_str == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria para numero.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(num_str, lexer->source_code + start_pos, length);
    num_str[length] = '\0';
    return create_token(TOKEN_NUMBER, num_str, lexer->line, start_column);
}

Token* read_string(Lexer* lexer) {
    int start_column = lexer->column;
    advance(lexer); // Consome a aspa inicial "'"
    int start_pos = lexer->current_pos;
    while (lexer->current_char != '\0' && lexer->current_char != '\'') {
        advance(lexer);
    }
    if (lexer->current_char == '\0') {
        fprintf(stderr, "Erro lexico: String nao terminada na linha %d, coluna %d.\n", lexer->line, start_column);
        return create_token(TOKEN_ERROR, "String nao terminada", lexer->line, start_column);
    }
    int length = lexer->current_pos - start_pos;
    char* str_value = (char*)malloc(length + 3); // +2 para aspas, +1 para '\0'
    if (str_value == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria para string.\n");
        exit(EXIT_FAILURE);
    }
    str_value[0] = '\'';
    strncpy(str_value + 1, lexer->source_code + start_pos, length);
    str_value[length + 1] = '\'';
    str_value[length + 2] = '\0';
    advance(lexer); // Consome a aspa final "'"
    return create_token(TOKEN_STRING_LITERAL, str_value, lexer->line, start_column);
}

// Mapeamento de palavras-chave (uma forma simples, pode ser otimizado com hash table)
TokenType get_keyword_type(const char* identifier) {
    if (strcmp(identifier, "func") == 0) return TOKEN_FUNC;
    if (strcmp(identifier, "if") == 0) return TOKEN_IF;
    if (strcmp(identifier, "else") == 0) return TOKEN_ELSE;
    if (strcmp(identifier, "while") == 0) return TOKEN_WHILE;
    if (strcmp(identifier, "break") == 0) return TOKEN_BREAK;
    if (strcmp(identifier, "int") == 0) return TOKEN_INT;
    if (strcmp(identifier, "char") == 0) return TOKEN_CHAR;
    return TOKEN_IDENTIFIER; // Se não for palavra-chave, é um identificador
}

Token* read_identifier_or_keyword(Lexer* lexer) {
    int start_column = lexer->column;
    int start_pos = lexer->current_pos;
    while (lexer->current_char != '\0' && (isalnum(lexer->current_char) || lexer->current_char == '_')) {
        advance(lexer);
    }
    int length = lexer->current_pos - start_pos;
    char* id_str = (char*)malloc(length + 1);
    if (id_str == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria para identificador.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(id_str, lexer->source_code + start_pos, length);
    id_str[length] = '\0';
    
    TokenType type = get_keyword_type(id_str);
    return create_token(type, id_str, lexer->line, start_column);
}

// --- Função Principal do Lexer ---

Token* get_next_token(Lexer* lexer) {
    while (lexer->current_char != '\0') {
        skip_whitespace(lexer); // Pula espaços em branco antes de cada token

        if (lexer->current_char == '\0') {
            break; // Fim do arquivo após pular espaços
        }

        int current_line = lexer->line;
        int current_column = lexer->column;

        // Tenta casar operadores de dois caracteres primeiro
        char next_char = peek(lexer, 1);
        if (lexer->current_char == '=' && next_char == '=') { advance(lexer); advance(lexer); return create_token(TOKEN_EQ, "==", current_line, current_column); }
        if (lexer->current_char == '!' && next_char == '=') { advance(lexer); advance(lexer); return create_token(TOKEN_NE, "!=", current_line, current_column); }
        if (lexer->current_char == '&' && next_char == '&') { advance(lexer); advance(lexer); return create_token(TOKEN_AND, "&&", current_line, current_column); }
        if (lexer->current_char == '|' && next_char == '|') { advance(lexer); advance(lexer); return create_token(TOKEN_OR, "||", current_line, current_column); }
        if (lexer->current_char == '<' && next_char == '=') { advance(lexer); advance(lexer); return create_token(TOKEN_LE, "<=", current_line, current_column); }
        if (lexer->current_char == '>' && next_char == '=') { advance(lexer); advance(lexer); return create_token(TOKEN_GE, ">=", current_line, current_column); }

        // Tenta casar operadores de um caractere e símbolos
        switch (lexer->current_char) {
            case '=': advance(lexer); return create_token(TOKEN_ASSIGN, "=", current_line, current_column);
            case ';': advance(lexer); return create_token(TOKEN_SEMICOLON, ";", current_line, current_column);
            case '(': advance(lexer); return create_token(TOKEN_LPAREN, "(", current_line, current_column);
            case ')': advance(lexer); return create_token(TOKEN_RPAREN, ")", current_line, current_column);
            case '{': advance(lexer); return create_token(TOKEN_LBRACE, "{", current_line, current_column);
            case '}': advance(lexer); return create_token(TOKEN_RBRACE, "}", current_line, current_column);
            case ',': advance(lexer); return create_token(TOKEN_COMMA, ",", current_line, current_column);
            case '+': advance(lexer); return create_token(TOKEN_PLUS, "+", current_line, current_column);
            case '-': advance(lexer); return create_token(TOKEN_MINUS, "-", current_line, current_column);
            case '*': advance(lexer); return create_token(TOKEN_MULTIPLY, "*", current_line, current_column);
            case '/': advance(lexer); return create_token(TOKEN_DIVIDE, "/", current_line, current_column);
            case '<': advance(lexer); return create_token(TOKEN_LT, "<", current_line, current_column);
            case '>': advance(lexer); return create_token(TOKEN_GT, ">", current_line, current_column);
            case '\'': return read_string(lexer);
        }

        // Tenta casar números
        if (isdigit(lexer->current_char)) {
            return read_number(lexer);
        }

        // Tenta casar identificadores ou palavras-chave
        if (isalpha(lexer->current_char) || lexer->current_char == '_') {
            return read_identifier_or_keyword(lexer);
        }

        // Caractere não reconhecido
        fprintf(stderr, "Erro lexico: Caractere nao reconhecido '%c' na linha %d, coluna %d.\n", lexer->current_char, lexer->line, lexer->column);
        advance(lexer); // Avança para evitar loop infinito
        return create_token(TOKEN_ERROR, "Caractere desconhecido", current_line, current_column);
    }
    return create_token(TOKEN_EOF, NULL, lexer->line, lexer->column);
}

// --- Inicialização do Lexer ---
Lexer* init_lexer(const char* source_code) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    if (lexer == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria para lexer.\n");
        exit(EXIT_FAILURE);
    }
    lexer->source_code = source_code;
    lexer->current_pos = 0;
    lexer->source_len = strlen(source_code);
    lexer->current_char = lexer->source_code[lexer->current_pos];
    lexer->line = 1;
    lexer->column = 1;
    return lexer;
}

// --- Função para imprimir o token (para depuração) ---
void print_token(Token* token) {
    const char* type_str;
    switch (token->type) {
        case TOKEN_FUNC: type_str = "FUNC"; break;
        case TOKEN_IF: type_str = "IF"; break;
        case TOKEN_ELSE: type_str = "ELSE"; break;
        case TOKEN_WHILE: type_str = "WHILE"; break;
        case TOKEN_BREAK: type_str = "BREAK"; break;
        case TOKEN_INT: type_str = "INT"; break;
        case TOKEN_CHAR: type_str = "CHAR"; break;
        case TOKEN_ASSIGN: type_str = "ASSIGN"; break;
        case TOKEN_SEMICOLON: type_str = "SEMICOLON"; break;
        case TOKEN_LPAREN: type_str = "LPAREN"; break;
        case TOKEN_RPAREN: type_str = "RPAREN"; break;
        case TOKEN_LBRACE: type_str = "LBRACE"; break;
        case TOKEN_RBRACE: type_str = "RBRACE"; break;
        case TOKEN_COMMA: type_str = "COMMA"; break;
        case TOKEN_PLUS: type_str = "PLUS"; break;
        case TOKEN_MINUS: type_str = "MINUS"; break;
        case TOKEN_MULTIPLY: type_str = "MULTIPLY"; break;
        case TOKEN_DIVIDE: type_str = "DIVIDE"; break;
        case TOKEN_AND: type_str = "AND"; break;
        case TOKEN_OR: type_str = "OR"; break;
        case TOKEN_NE: type_str = "NE"; break;
        case TOKEN_GT: type_str = "GT"; break;
        case TOKEN_LT: type_str = "LT"; break;
        case TOKEN_EQ: type_str = "EQ"; break;
        case TOKEN_LE: type_str = "LE"; break;
        case TOKEN_GE: type_str = "GE"; break;
        case TOKEN_NUMBER: type_str = "NUMBER"; break;
        case TOKEN_STRING_LITERAL: type_str = "STRING_LITERAL"; break;
        case TOKEN_IDENTIFIER: type_str = "IDENTIFIER"; break;
        case TOKEN_EOF: type_str = "EOF"; break;
        case TOKEN_ERROR: type_str = "ERROR"; break;
        default: type_str = "UNKNOWN"; break;
    }
    printf("Token(%s, '%s', L%d, C%d)\n", type_str, (token->value ? token->value : "NULL"), token->line, token->column);
}

char* read_file_to_char_array(const char* filename, long* file_size) {
    FILE* file = NULL;
    char* buffer = NULL;
    long length = 0;

    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return NULL;
    }

    // 2. Descobrir o tamanho do arquivo
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 3. Alocar memória para o buffer + 1 para o terminador nulo
    buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        perror("Erro ao alocar memoria");
        fclose(file);
        return NULL;
    }

    // 4. Ler o conteúdo do arquivo para o buffer
    size_t bytes_read = fread(buffer, 1, length, file);
    if (bytes_read != length) {
        fprintf(stderr, "Erro ao ler o arquivo: Apenas %zu de %ld bytes foram lidos.\n", bytes_read, length);
        free(buffer);
        fclose(file);
        return NULL;
    }

    // 5. Adicionar o terminador nulo
    buffer[length] = '\0';

    fclose(file);

    // 7. Retornar o tamanho do arquivo através do ponteiro
    if (file_size != NULL) {
        *file_size = length;
    }

    return buffer;
}

int main(int argc, char *argv[]) {

    if(argc < 2){
        printf("Nenhum arquivo informado.\n");
        return 1;
    }

    const char* filename = argv[1];
    char* file_content = NULL;
    long content_length = 0;

    file_content = read_file_to_char_array(filename, &content_length);

    if (file_content == NULL) {
        fprintf(stderr, "Falha ao ler o arquivo.\n");
        return 1;
    }

    Lexer* lexer = init_lexer(file_content);
    Token* token;

    do {
        token = get_next_token(lexer);
        print_token(token);
        // Se for um token de erro fatal ou EOF, pare de processar
        if (token->type == TOKEN_ERROR || token->type == TOKEN_EOF) {
            free_token(token);
            break;
        }
        free_token(token); // Libere a memória do token após usá-lo
    } while (1); // Loop infinito, quebra com break dentro do if

    free(lexer); // Libera a memória do lexer
    return 0;
}