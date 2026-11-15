/*
 * Banking Simulation System
 * Console-based mini project in C.
 * Educational simulation only; not real banking software.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#define DATA_FILE "accounts.dat"
#define NAME_LEN 60
#define PIN_LEN 5
#define MAX_TX 100
#define MIN_OPENING_BALANCE 500.0
#define MAX_LOGIN_ATTEMPTS 3

typedef struct {
    char date[20];
    char type[20];
    double amount;
    double balance_after;
} Transaction;

typedef struct {
    int account_no;
    char name[NAME_LEN];
    char pin[PIN_LEN];
    double balance;
    int locked;
    int transaction_count;
    Transaction transactions[MAX_TX];
} Account;

static Account *accounts = NULL;
static size_t account_count = 0;

static void clear_input_buffer(void);
static int read_int(const char *prompt, int *value);
static int read_amount(const char *prompt, double *value);
static void read_line(const char *prompt, char *buffer, size_t size);
static int valid_pin(const char *pin);
static void load_accounts(void);
static int save_accounts(void);
static int next_account_number(void);
static int find_account_index(int account_no);
static void add_transaction(Account *account, const char *type, double amount);
static void create_account(void);
static int login(void);
static void account_menu(size_t index);
static void deposit(size_t index);
static void withdraw_money(size_t index);
static void transfer_money(size_t index);
static void check_balance(size_t index);
static void change_pin(size_t index);
static void show_history(size_t index);
static void pause_screen(void);

static void clear_input_buffer(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
}

static int read_int(const char *prompt, int *value)
{
    char line[100], *end;
    long parsed;
    printf("%s", prompt);
    if (!fgets(line, sizeof line, stdin)) return 0;
    errno = 0;
    parsed = strtol(line, &end, 10);
    while (isspace((unsigned char)*end)) end++;
    if (errno || end == line || *end != '\0' ||
        parsed < -2147483647L || parsed > 2147483647L) {
        printf("Invalid input. Please enter a whole number.\n");
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int read_amount(const char *prompt, double *value)
{
    char line[100], *end;
    double parsed;
    printf("%s", prompt);
    if (!fgets(line, sizeof line, stdin)) return 0;
    errno = 0;
    parsed = strtod(line, &end);
    while (isspace((unsigned char)*end)) end++;
    if (errno || end == line || *end != '\0' || parsed <= 0.0 ||
        parsed > 1000000000000.0) {
        printf("Please enter a valid amount greater than zero.\n");
        return 0;
    }
    *value = parsed;
    return 1;
}

static void read_line(const char *prompt, char *buffer, size_t size)
{
    size_t len;
    printf("%s", prompt);
    if (!fgets(buffer, (int)size, stdin)) {
        buffer[0] = '\0';
        return;
    }
    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
    else clear_input_buffer();
}

static int valid_pin(const char *pin)
{
    size_t i;
    if (strlen(pin) != 4) return 0;
    for (i = 0; i < 4; i++)
        if (!isdigit((unsigned char)pin[i])) return 0;
    return 1;
}

static void load_accounts(void)
{
    FILE *file = fopen(DATA_FILE, "rb");
    Account temp;
    size_t capacity = 0;
    if (!file) return;
    while (fread(&temp, sizeof(Account), 1, file) == 1) {
        if (account_count == capacity) {
            size_t new_capacity = capacity == 0 ? 4 : capacity * 2;
            Account *grown = realloc(accounts, new_capacity * sizeof(Account));
            if (!grown) {
                fprintf(stderr, "Not enough memory to load accounts.\n");
                free(accounts);
                accounts = NULL;
                account_count = 0;
                fclose(file);
                return;
            }
            accounts = grown;
            capacity = new_capacity;
        }
        accounts[account_count++] = temp;
    }
    fclose(file);
}

static int save_accounts(void)
{
    FILE *file = fopen(DATA_FILE, "wb");
    if (!file) {
        perror("Could not save account data");
        return 0;
    }
    if (account_count > 0 &&
        fwrite(accounts, sizeof(Account), account_count, file) != account_count) {
        perror("Could not write account data");
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}

static int next_account_number(void)
{
    int highest = 1000;
    size_t i;
    for (i = 0; i < account_count; i++)
        if (accounts[i].account_no > highest) highest = accounts[i].account_no;
    return highest + 1;
}

static int find_account_index(int account_no)
{
    size_t i;
    for (i = 0; i < account_count; i++)
        if (accounts[i].account_no == account_no) return (int)i;
    return -1;
}

static void add_transaction(Account *account, const char *type, double amount)
{
    Transaction *tx;
    time_t now;
    struct tm *local;
    if (account->transaction_count >= MAX_TX) {
        memmove(&account->transactions[0], &account->transactions[1],
                (MAX_TX - 1) * sizeof(Transaction));
        account->transaction_count = MAX_TX - 1;
    }
    tx = &account->transactions[account->transaction_count++];
    now = time(NULL);
    local = localtime(&now);
    if (local) strftime(tx->date, sizeof tx->date, "%Y-%m-%d %H:%M:%S", local);
    else strcpy(tx->date, "Unknown time");
    snprintf(tx->type, sizeof tx->type, "%s", type);
    tx->amount = amount;
    tx->balance_after = account->balance;
}

static void create_account(void)
{
    Account new_account;
    Account *grown;
    char pin[100];
    double opening_balance;
    memset(&new_account, 0, sizeof new_account);
    read_line("Enter full name: ", new_account.name, sizeof new_account.name);
    if (new_account.name[0] == '\0') {
        printf("Name cannot be empty.\n");
        return;
    }
    read_line("Set a 4-digit PIN: ", pin, sizeof pin);
    if (!valid_pin(pin)) {
        printf("Invalid PIN. It must contain exactly 4 digits.\n");
        return;
    }
    strcpy(new_account.pin, pin);
    if (!read_amount("Enter opening deposit (minimum Rs. 500): ", &opening_balance))
        return;
    if (opening_balance < MIN_OPENING_BALANCE) {
        printf("Opening deposit must be at least Rs. %.2f.\n", MIN_OPENING_BALANCE);
        return;
    }
    new_account.account_no = next_account_number();
    new_account.balance = opening_balance;
    new_account.locked = 0;
    new_account.transaction_count = 0;
    add_transaction(&new_account, "OPENING", opening_balance);
    grown = realloc(accounts, (account_count + 1) * sizeof(Account));
    if (!grown) {
        printf("Unable to create account: not enough memory.\n");
        return;
    }
    accounts = grown;
    accounts[account_count++] = new_account;
    if (save_accounts()) {
        printf("\nAccount created successfully!\n");
        printf("Account holder : %s\n", new_account.name);
        printf("Account number : %d\n", new_account.account_no);
        printf("Balance        : Rs. %.2f\n", new_account.balance);
    }
}

static int login(void)
{
    int account_no, index, attempts = 0;
    char pin[100];
    if (!read_int("Enter account number: ", &account_no)) return -1;
    index = find_account_index(account_no);
    if (index < 0) {
        printf("Account not found.\n");
        return -1;
    }
    if (accounts[index].locked) {
        printf("This account is locked due to repeated incorrect PIN attempts.\n");
        return -1;
    }
    while (attempts < MAX_LOGIN_ATTEMPTS) {
        read_line("Enter 4-digit PIN: ", pin, sizeof pin);
        if (strcmp(pin, accounts[index].pin) == 0) {
            printf("\nLogin successful. Welcome, %s!\n", accounts[index].name);
            return index;
        }
        attempts++;
        printf("Incorrect PIN. Attempts remaining: %d\n",
               MAX_LOGIN_ATTEMPTS - attempts);
    }
    accounts[index].locked = 1;
    save_accounts();
    printf("Account locked after 3 incorrect attempts.\n");
    return -1;
}

static void deposit(size_t index)
{
    double amount;
    if (!read_amount("Enter deposit amount: Rs. ", &amount)) return;
    accounts[index].balance += amount;
    add_transaction(&accounts[index], "DEPOSIT", amount);
    if (save_accounts())
        printf("Deposit successful. New balance: Rs. %.2f\n", accounts[index].balance);
}

static void withdraw_money(size_t index)
{
    double amount;
    if (!read_amount("Enter withdrawal amount: Rs. ", &amount)) return;
    if (amount > accounts[index].balance) {
        printf("Insufficient balance. Withdrawal rejected.\n");
        return;
    }
    accounts[index].balance -= amount;
    add_transaction(&accounts[index], "WITHDRAW", amount);
    if (save_accounts())
        printf("Withdrawal successful. New balance: Rs. %.2f\n", accounts[index].balance);
}

static void transfer_money(size_t index)
{
    int recipient_no, recipient_index;
    double amount;
    if (!read_int("Enter recipient account number: ", &recipient_no)) return;
    if (recipient_no == accounts[index].account_no) {
        printf("You cannot transfer money to your own account.\n");
        return;
    }
    recipient_index = find_account_index(recipient_no);
    if (recipient_index < 0) {
        printf("Recipient account not found.\n");
        return;
    }
    if (accounts[recipient_index].locked) {
        printf("Recipient account is locked. Transfer rejected.\n");
        return;
    }
    printf("Recipient name: %s\n", accounts[recipient_index].name);
    if (!read_amount("Enter transfer amount: Rs. ", &amount)) return;
    if (amount > accounts[index].balance) {
        printf("Insufficient balance. Transfer rejected.\n");
        return;
    }
    accounts[index].balance -= amount;
    accounts[recipient_index].balance += amount;
    add_transaction(&accounts[index], "TRANSFER OUT", amount);
    add_transaction(&accounts[recipient_index], "TRANSFER IN", amount);
    if (save_accounts())
        printf("Transfer successful. Your new balance: Rs. %.2f\n", accounts[index].balance);
}

static void check_balance(size_t index)
{
    printf("\nAccount holder : %s\n", accounts[index].name);
    printf("Account number : %d\n", accounts[index].account_no);
    printf("Available balance: Rs. %.2f\n", accounts[index].balance);
}

static void change_pin(size_t index)
{
    char current_pin[100], new_pin[100];
    read_line("Enter current PIN: ", current_pin, sizeof current_pin);
    if (strcmp(current_pin, accounts[index].pin) != 0) {
        printf("Current PIN is incorrect.\n");
        return;
    }
    read_line("Enter new 4-digit PIN: ", new_pin, sizeof new_pin);
    if (!valid_pin(new_pin)) {
        printf("New PIN must contain exactly 4 digits.\n");
        return;
    }
    if (strcmp(new_pin, accounts[index].pin) == 0) {
        printf("New PIN must be different from the current PIN.\n");
        return;
    }
    strcpy(accounts[index].pin, new_pin);
    if (save_accounts()) printf("PIN changed successfully.\n");
}

static void show_history(size_t index)
{
    int i;
    Account *account = &accounts[index];
    printf("\n%-20s %-16s %-14s %-14s\n",
           "Date & Time", "Transaction", "Amount (Rs.)", "Balance (Rs.)");
    printf("--------------------------------------------------------------------------\n");
    if (account->transaction_count == 0) {
        printf("No transactions found.\n");
        return;
    }
    for (i = 0; i < account->transaction_count; i++) {
        Transaction *tx = &account->transactions[i];
        printf("%-20s %-16s %-14.2f %-14.2f\n",
               tx->date, tx->type, tx->amount, tx->balance_after);
    }
}

static void account_menu(size_t index)
{
    int choice;
    do {
        printf("\n========== ACCOUNT MENU ==========\n");
        printf("1. Deposit money\n2. Withdraw money\n3. Transfer money\n");
        printf("4. Balance inquiry\n5. Change PIN\n6. View transaction history\n0. Logout\n");
        if (!read_int("Enter your choice: ", &choice)) continue;
        switch (choice) {
            case 1: deposit(index); break;
            case 2: withdraw_money(index); break;
            case 3: transfer_money(index); break;
            case 4: check_balance(index); break;
            case 5: change_pin(index); break;
            case 6: show_history(index); break;
            case 0: printf("Logged out successfully.\n"); break;
            default: printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);
}

static void pause_screen(void)
{
    char line[8];
    printf("\nPress Enter to continue...");
    (void)fgets(line, sizeof line, stdin);
}

int main(void)
{
    int choice, logged_in_index;
    load_accounts();
    printf("=====================================\n");
    printf("       BANKING SIMULATION SYSTEM\n");
    printf("=====================================\n");
    do {
        printf("\n============== MAIN MENU ==============\n");
        printf("1. Create new account\n2. Login\n0. Exit\n");
        if (!read_int("Enter your choice: ", &choice)) continue;
        switch (choice) {
            case 1:
                create_account();
                pause_screen();
                break;
            case 2:
                logged_in_index = login();
                if (logged_in_index >= 0) account_menu((size_t)logged_in_index);
                pause_screen();
                break;
            case 0:
                printf("Thank you for using Banking Simulation System.\n");
                break;
            default:
                printf("Invalid choice. Please enter 0, 1 or 2.\n");
        }
    } while (choice != 0);
    save_accounts();
    free(accounts);
    return 0;
}
