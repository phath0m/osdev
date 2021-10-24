#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

typedef enum {
    UNARY_OP_STRING_ZERO,
    UANRY_OP_STRING_NOT_ZERO,
    UNARY_OP_FILE_EXISTS,
    UNARY_OP_REG_FILE_EXISTS,
    UNARY_OP_DIR_EXISTS,
    UNARY_OP_FILE_CAN_READ,
    UNARY_OP_FILE_CAN_WRITE,
    UNARY_OP_FILE_CAN_EXECUTE,
    UNARY_OP_FILE_IS_NOT_EMPTY,
    UNARY_OP_FILE_IS_OWNED_BY_EUID,
    UNARY_OP_FILE_IS_OWNED_BY_EGID,
    UNARY_OP_BLOCK_FILE_EXISTS,
    UNARY_OP_CHARACTER_FILE_EXISTS,
    UNARY_OP_SOCKET_FILE_EXISTS,
    UNARY_OP_SYMLINK_EXISTS
} unary_op_t;

typedef enum {
    BINOP_EQUAL = 0,
    BINOP_NOT_EQUAL = 1,
    BINOP_GE = 2,
    BINOP_GT = 3,
    BINOP_LE = 4,
    BINOP_LT = 5,
    BINOP_STR_EQ = 6,
    BINOP_STR_NE = 7
} binary_op_t;


static binary_op_t
parse_binary_op(const char *op)
{
    const char *binop_lut[] = {"-eq", "-ne", "-ge", "-gt", "-le", "-lt", "=", "!="};

    int i;

    for (i = 0; binop_lut[i]; i++) {
        if (strcmp(op, binop_lut[i]) == 0) return (binary_op_t)i;
    }

    fprintf(stderr, "binary operator expected\n");

    return 0;
}

static unary_op_t
parse_unary_op(const char *op)
{
    if (strlen(op) != 2 || op[0] != '-') {
        goto error;
    }

    switch (op[1]) {
        case 'e': return UNARY_OP_FILE_EXISTS;
        case 'f': return UNARY_OP_REG_FILE_EXISTS;
        case 'd': return UNARY_OP_DIR_EXISTS;
        case 'r': return UNARY_OP_FILE_CAN_READ;
        case 'w': return UNARY_OP_FILE_CAN_WRITE;
        case 'x': return UNARY_OP_FILE_CAN_EXECUTE;
        case 's': return UNARY_OP_FILE_IS_NOT_EMPTY;
        case 'O': return UNARY_OP_FILE_IS_OWNED_BY_EUID;
        case 'G': return UNARY_OP_FILE_IS_OWNED_BY_EGID;
        case 'L':
        case 'h': return UNARY_OP_SYMLINK_EXISTS;
        case 'b': return UNARY_OP_BLOCK_FILE_EXISTS;
        case 'c': return UNARY_OP_CHARACTER_FILE_EXISTS;
        case 'S': return UNARY_OP_SOCKET_FILE_EXISTS;
        default:
            break;
    }

error:
    fprintf(stderr, "unary operator expected\n");
    return 0;
}

#define TEST_EXPR(expr) ((expr) ? 0 : 1)

static int
test_numeric_bin_op(binary_op_t binop, const char *left_s, const char *right_s)
{
    int left;
    int right;

    left = atoi(left_s);
    right = atoi(right_s);

    switch (binop) {
        case BINOP_EQUAL:
            return TEST_EXPR(left == right);
        case BINOP_GE:
            return TEST_EXPR(left >= right);
        case BINOP_GT:
            return TEST_EXPR(left > right);
        case BINOP_LE:
            return TEST_EXPR(left <= right);
        case BINOP_LT:
            return TEST_EXPR(left < right);
        case BINOP_NOT_EQUAL:
            return TEST_EXPR(left != right);
        default:
            return -1;
    }
}

static int
test_string_binop(binary_op_t binop, const char *left, const char *right)
{
    switch (binop) {
        case BINOP_STR_EQ:
            return TEST_EXPR(strcmp(left, right));
        case BINOP_STR_NE:
            return TEST_EXPR(strcmp(left, right));
        default:
            return -1;
    }
}

static int
test_bin_op(binary_op_t binop, const char *left, const char *right)
{
    switch (binop) {
        case BINOP_EQUAL:
        case BINOP_GE:
        case BINOP_GT:
        case BINOP_LE:
        case BINOP_LT:
        case BINOP_NOT_EQUAL:
            return test_numeric_bin_op(binop, left, right);
        case BINOP_STR_EQ:
        case BINOP_STR_NE:
            return test_string_binop(binop, left, right);
        default:
            return -1;
    }
}

static int
test_file_unary_op(unary_op_t op, const char *file)
{
    struct stat sb;

    switch (op) {
        case UNARY_OP_FILE_CAN_READ:
            return TEST_EXPR(access(file, R_OK) == 0);
        case UNARY_OP_FILE_CAN_WRITE:
            return TEST_EXPR(access(file, W_OK) == 0);
        case UNARY_OP_FILE_CAN_EXECUTE:
            return TEST_EXPR(access(file, X_OK) == 0);
        default:
            break;
    }

    if (stat(file, &sb) != 0) {
        return 1;
    }

    switch (op) {
        case UNARY_OP_FILE_EXISTS:
            return 0;
        case UNARY_OP_REG_FILE_EXISTS:
            return TEST_EXPR((S_IFMT & sb.st_mode) == S_IFREG);
        case UNARY_OP_DIR_EXISTS:
            return TEST_EXPR((S_IFMT & sb.st_mode) == S_IFDIR);
        case UNARY_OP_CHARACTER_FILE_EXISTS:
            return TEST_EXPR((S_IFMT & sb.st_mode) == S_IFCHR);
        case UNARY_OP_BLOCK_FILE_EXISTS:
            return TEST_EXPR((S_IFMT & sb.st_mode) == S_IFBLK);
        case UNARY_OP_SOCKET_FILE_EXISTS:
            return TEST_EXPR((S_IFMT & sb.st_mode) == S_IFSOCK);
        case UNARY_OP_SYMLINK_EXISTS:
            return TEST_EXPR((S_IFMT & sb.st_mode) == S_IFLNK);
        case UNARY_OP_FILE_IS_OWNED_BY_EGID:
            return TEST_EXPR(sb.st_gid == getegid());
        case UNARY_OP_FILE_IS_OWNED_BY_EUID:
            return TEST_EXPR(sb.st_uid == geteuid());
        case UNARY_OP_FILE_IS_NOT_EMPTY:
            return TEST_EXPR(sb.st_size != 0);
        default:
            return -1;
    }
}

static int
test_unary_op(unary_op_t op, const char *val)
{
    switch (op) {
        case UNARY_OP_FILE_CAN_READ:
        case UNARY_OP_FILE_CAN_WRITE:
        case UNARY_OP_FILE_CAN_EXECUTE:
        case UNARY_OP_FILE_EXISTS:
        case UNARY_OP_REG_FILE_EXISTS:
        case UNARY_OP_DIR_EXISTS:
        case UNARY_OP_CHARACTER_FILE_EXISTS:
        case UNARY_OP_BLOCK_FILE_EXISTS:
        case UNARY_OP_SOCKET_FILE_EXISTS:
        case UNARY_OP_SYMLINK_EXISTS:
        case UNARY_OP_FILE_IS_OWNED_BY_EGID:
        case UNARY_OP_FILE_IS_OWNED_BY_EUID:
        case UNARY_OP_FILE_IS_NOT_EMPTY:
                return test_file_unary_op(op, val);
        default:
            return -1;   
    }
}

int
main(int argc, const char *argv[])
{
    binary_op_t binop;
    unary_op_t unaryop;

    if (argc == 3) {
        unaryop = parse_unary_op(argv[1]);

        return test_unary_op(unaryop, argv[2]);

    } else if (argc == 4) {
        binop = parse_binary_op(argv[2]);

        return test_bin_op(binop, argv[1], argv[3]);
    } else {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        return -1;
    }

    return 0;
}