/*
 * bus.c - Member 1 : Bus Management
 * File: buses.txt   format: busNo|type|seats|mileage|lastService|status
 *
 * - Add bus (with validation)
 * - View buses with service status
 * - Update mileage / record service
 * - Change status (Active / Repair)
 * - Delete bus and its assigned trips
 * - Service due report (sorted)
 */
#include "common.h"

#define SERVICE_KM 10000.0f    /* service needed every 10,000 km */

int loadBuses(Bus b[]) {
    FILE *fp = fopen(BUS_FILE, "r");
    int n = 0;
    if (fp == NULL) return 0;
    while (n < MAX_REC && fscanf(fp, " %9[^|]|%9[^|]|%d|%f|%f|%9[^\n]", b[n].busNo,
                                 b[n].type, &b[n].seats, &b[n].mileage,
                                 &b[n].lastService, b[n].status) == 6)
        n++;
    fclose(fp);
    return n;
}

void saveBuses(Bus b[], int n) {
    FILE *fp = fopen(BUS_FILE, "w");
    int i;
    for (i = 0; i < n; i++)
        fprintf(fp, "%s|%s|%d|%.0f|%.0f|%s\n", b[i].busNo, b[i].type, b[i].seats,
                b[i].mileage, b[i].lastService, b[i].status);
    fclose(fp);
}

int findBus(Bus b[], int n, char busNo[]) {
    int i;
    for (i = 0; i < n; i++)
        if (strcmp(b[i].busNo, busNo) == 0) return i;
    return -1;
}

/* km left until next service (negative = overdue) */
static float kmLeft(Bus b) {
    return b.lastService + SERVICE_KM - b.mileage;
}

static void printBus(Bus b) {
    printf("%-10s %-7s %5d %10.0f  %-7s ", b.busNo, b.type, b.seats, b.mileage, b.status);
    if (kmLeft(b) <= 0) printf("SERVICE OVERDUE\n");
    else printf("%.0f km to service\n", kmLeft(b));
}

static void addBus(void) {
    Bus b[MAX_REC], x;
    int n = loadBuses(b);

    readText("Bus number (e.g. NB-4521): ", x.busNo, sizeof(x.busNo));
    if (findBus(b, n, x.busNo) != -1) {
        printf("This bus already exists.\n");
        return;
    }
    if (readInt("Type (1-Normal 2-AC): ", 1, 2) == 1) strcpy(x.type, "Normal");
    else strcpy(x.type, "AC");
    x.seats = readInt("Seats (20-60): ", 20, MAX_SEATS);
    x.mileage = readFloat("Mileage (km): ", 0, 2000000);
    x.lastService = x.mileage;
    strcpy(x.status, "Active");

    b[n] = x;
    saveBuses(b, n + 1);
    printf("Bus %s added.\n", x.busNo);
}

static void viewBuses(void) {
    Bus b[MAX_REC];
    int n = loadBuses(b), i;
    printf("\n%-10s %-7s %5s %10s  %-7s %s\n", "Bus", "Type", "Seats", "Mileage", "Status", "Service");
    for (i = 0; i < n; i++) printBus(b[i]);
    printf("Total buses: %d\n", n);
}

static void updateMileage(void) {
    Bus b[MAX_REC];
    char no[10];
    int n = loadBuses(b), i;

    readText("Bus number: ", no, sizeof(no));
    i = findBus(b, n, no);
    if (i == -1) { printf("Bus not found.\n"); return; }

    printf("Current mileage: %.0f km\n", b[i].mileage);
    b[i].mileage = readFloat("New mileage: ", b[i].mileage, 2000000);
    if (readInt("Was the bus serviced now? (1-Yes 0-No): ", 0, 1) == 1)
        b[i].lastService = b[i].mileage;
    saveBuses(b, n);
    printBus(b[i]);
}

static void changeStatus(void) {
    Bus b[MAX_REC];
    char no[10];
    int n = loadBuses(b), i;

    readText("Bus number: ", no, sizeof(no));
    i = findBus(b, n, no);
    if (i == -1) { printf("Bus not found.\n"); return; }

    if (readInt("New status (1-Active 2-Repair): ", 1, 2) == 1) strcpy(b[i].status, "Active");
    else strcpy(b[i].status, "Repair");
    saveBuses(b, n);
    printf("Status changed to %s.\n", b[i].status);
}

static void deleteBus(void) {
    Bus b[MAX_REC];
    Trip t[MAX_REC];
    char no[10];
    int nb = loadBuses(b), nt = loadTrips(t), idx, i, j;

    if (nb == 0) {
        printf("No buses available to delete.\n");
        return;
    }

    viewBuses();
    readText("Enter Bus number to delete: ", no, sizeof(no));
    idx = findBus(b, nb, no);

    if (idx == -1) {
        printf("Bus %s not found.\n", no);
        return;
    }

    /* Remove bus from array */
    for (i = idx; i < nb - 1; i++) {
        b[i] = b[i + 1];
    }
    saveBuses(b, nb - 1);

    /* Remove any trips assigned to this bus */
    Trip newTrips[MAX_REC];
    int newNt = 0;
    for (i = 0; i < nt; i++) {
        if (strcmp(t[i].busNo, no) != 0) {
            newTrips[newNt++] = t[i];
        }
    }
    saveTrips(newTrips, newNt);

    printf("Bus %s and its assigned trips deleted successfully.\n", no);
}

/* Buses that need service, most urgent first (bubble sort) */
static void serviceReport(void) {
    Bus b[MAX_REC], temp;
    int n = loadBuses(b), i, j;

    for (i = 0; i < n - 1; i++)
        for (j = 0; j < n - 1 - i; j++)
            if (kmLeft(b[j]) > kmLeft(b[j + 1])) {
                temp = b[j]; b[j] = b[j + 1]; b[j + 1] = temp;
            }

    printf("\nBuses needing service within 1000 km:\n");
    for (i = 0; i < n; i++)
        if (kmLeft(b[i]) <= 1000) printBus(b[i]);
}

void busMenu(void) {
    int c;
    do {
        printf("\n--- BUS MANAGEMENT ---\n");
        printf("1. Add bus\n2. View buses\n3. Update mileage\n4. Change status\n5. Delete bus\n6. Service report\n0. Back\n");
        c = readInt("Choice: ", 0, 6);
        if (c == 1) addBus();
        else if (c == 2) viewBuses();
        else if (c == 3) updateMileage();
        else if (c == 4) changeStatus();
        else if (c == 5) deleteBus();
        else if (c == 6) serviceReport();
    } while (c != 0);
}
