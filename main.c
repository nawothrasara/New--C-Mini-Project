/*
 * main.c - Main menu, login and input helper functions
 *
 * Compile : gcc *.c -o busys
 * Run     : ./busys
 */
#include "common.h"
#include <time.h>

User currentUser;

/* Function prototypes for menus */
static void adminMenu(void);
static void clerkMenu(void);

/* ---------------- Input helpers (used by all files) ---------------- */

/* Read a line of text. Empty text and '|' are not allowed. */
void readText(char msg[], char out[], int size) {
    char line[200];
    while (1) {
        printf("%s", msg);
        if (fgets(line, sizeof(line), stdin) == NULL) exit(0);
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0 || strchr(line, '|') != NULL) {
            printf("  Invalid input. Try again.\n");
            continue;
        }
        strncpy(out, line, size - 1);
        out[size - 1] = '\0';
        return;
    }
}

/* Read a whole number between min and max */
int readInt(char msg[], int min, int max) {
    char line[50];
    int value;
    char extra;
    while (1) {
        readText(msg, line, sizeof(line));
        if (sscanf(line, "%d %c", &value, &extra) == 1 && value >= min && value <= max)
            return value;
        printf("  Enter a number between %d and %d.\n", min, max);
    }
}

/* Read a decimal number between min and max */
float readFloat(char msg[], float min, float max) {
    char line[50];
    float value;
    char extra;
    while (1) {
        readText(msg, line, sizeof(line));
        if (sscanf(line, "%f %c", &value, &extra) == 1 && value >= min && value <= max)
            return value;
        printf("  Enter a number between %.0f and %.0f.\n", min, max);
    }
}

/* Read a date as YYYY-MM-DD and return it as YYYYMMDD */
int readDate(char msg[]) {
    char line[20];
    int y, m, d;
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    while (1) {
        readText(msg, line, sizeof(line));
        if (sscanf(line, "%d-%d-%d", &y, &m, &d) == 3 &&
            y >= 2024 && y <= 2100 && m >= 1 && m <= 12) {
            if (y % 4 == 0) days[1] = 29; else days[1] = 28;
            if (d >= 1 && d <= days[m - 1]) return y * 10000 + m * 100 + d;
        }
        printf("  Invalid date. Use YYYY-MM-DD.\n");
    }
}

/* Today's date as YYYYMMDD */
int today(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    return (t->tm_year + 1900) * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;
}

/* Number of days from d1 to d2 (both YYYYMMDD) */
static long dayNumber(int d) {
    long y = d / 10000, m = (d / 100) % 100, day = d % 100;
    if (m <= 2) { y--; m += 12; }
    return 365 * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + day;
}
int daysBetween(int d1, int d2) {
    return (int)(dayNumber(d2) - dayNumber(d1));
}

void printDate(int d) {
    printf("%04d-%02d-%02d", d / 10000, (d / 100) % 100, d % 100);
}

/* ---------------- Staff Login & Register ---------------- */
static int login(void) {
    FILE *fp;
    User u;
    char name[20], pass[20];
    int attempt;

    for (attempt = 1; attempt <= 3; attempt++) {
        readText("Username: ", name, sizeof(name));
        readText("Password: ", pass, sizeof(pass));
        fp = fopen(USER_FILE, "r");
        if (fp == NULL) { printf("users.txt not found.\n"); return 0; }
        while (fscanf(fp, " %19[^|]|%19[^|]|%9[^\n]", u.username, u.password, u.role) == 3) {
            if (strcmp(u.username, name) == 0 && strcmp(u.password, pass) == 0) {
                fclose(fp);
                currentUser = u;
                printf("\nWelcome %s (%s)\n", u.username, u.role);
                return 1;
            }
        }
        fclose(fp);
        printf("Wrong username or password (%d of 3)\n", attempt);
    }
    return 0;
}

static void registerStaff(void) {
    FILE *fp;
    User u;
    char name[20], pass[20];

    printf("\n--- STAFF REGISTRATION ---\n");
    readText("Username: ", name, sizeof(name));
    
    // Check if username exists
    fp = fopen(USER_FILE, "r");
    if (fp != NULL) {
        while (fscanf(fp, " %19[^|]|%19[^|]|%9[^\n]", u.username, u.password, u.role) == 3) {
            if (strcmp(u.username, name) == 0) {
                fclose(fp);
                printf("Username already exists! Please try another.\n");
                return;
            }
        }
        fclose(fp);
    }

    readText("Password: ", pass, sizeof(pass));

    fp = fopen(USER_FILE, "a");
    if (fp == NULL) {
        printf("Cannot open users.txt file.\n");
        return;
    }
    // Default role for registered staff is CLERK
    fprintf(fp, "%s|%s|ADMIN\n", name, pass);
    fclose(fp);

    printf("Staff member registered successfully! You can now login.\n");
}

static void staffMenu(void) {
    int choice;
    do {
        printf("\n--- STAFF SECTION ---\n");
        printf("1. Login\n2. Register\n0. Back\n");
        choice = readInt("Choice: ", 0, 2);
        if (choice == 1) {
            if (login()) {
                if (strcmp(currentUser.role, "ADMIN") == 0) adminMenu();
                else clerkMenu();
                strcpy(currentUser.username, "");
            }
        } else if (choice == 2) {
            registerStaff();
        }
    } while (choice != 0);
}

/* ---------------- Menus ---------------- */
static void adminMenu(void) {
    int c;
    do {
        printf("\n===== ADMIN MENU =====\n");
        printf("1. Bus Management\n2. Route & Trip Management\n3. Reports\n0. Logout\n");
        c = readInt("Choice: ", 0, 3);
        if (c == 1) busMenu();
        else if (c == 2) routeMenu();
        else if (c == 3) reportMenu();
    } while (c != 0);
}

static void clerkMenu(void) {
    int c;
    do {
        printf("\n===== CLERK MENU =====\n");
        printf("1. Passengers\n2. Search Trips\n3. Reservations\n0. Logout\n");
        c = readInt("Choice: ", 0, 3);
        if (c == 1) passengerMenu();
        else if (c == 2) searchTrips();
        else if (c == 3) reservationMenu();
    } while (c != 0);
}

int main(void) {
    int c;
    do {
        printf("\n==========================================\n");
        printf("  BUS RESERVATION & TRANSPORT SYSTEM\n");
        printf("==========================================\n");
        printf("1. Staff\n2. Passenger Kiosk\n0. Exit\n");
        c = readInt("Choice: ", 0, 2);
        if (c == 1) {
            staffMenu();
        } else if (c == 2) {
            kioskMenu();
        }
    } while (c != 0);
    printf("Goodbye.\n");
    return 0;
}
