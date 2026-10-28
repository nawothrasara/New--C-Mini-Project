/*
 * passenger.c - Member 4 : Passenger Management & Kiosk
 * File: passengers.txt  format: passengerId|name|nic|phone|trips
 *
 * - Register passenger (NIC and phone validation, no duplicates)
 * - Search by NIC, view all, booking history
 * - Loyalty: 10 or more trips -> 5% discount
 * - Kiosk: passengers search trips and reserve a seat (pay at counter)
 */
#include "common.h"
#include <ctype.h>

int loadPassengers(Passenger p[]) {
    FILE *fp = fopen(PASSENGER_FILE, "r");
    int n = 0;
    if (fp == NULL) return 0;
    while (n < MAX_REC && fscanf(fp, " %5[^|]|%29[^|]|%12[^|]|%10[^|]|%d", p[n].passengerId,
                                 p[n].name, p[n].nic, p[n].phone, &p[n].trips) == 5)
        n++;
    fclose(fp);
    return n;
}

void savePassengers(Passenger p[], int n) {
    FILE *fp = fopen(PASSENGER_FILE, "w");
    int i;
    for (i = 0; i < n; i++)
        fprintf(fp, "%s|%s|%s|%s|%d\n", p[i].passengerId, p[i].name, p[i].nic,
                p[i].phone, p[i].trips);
    fclose(fp);
}

int findPassengerByNIC(Passenger p[], int n, char nic[]) {
    int i;
    for (i = 0; i < n; i++)
        if (strcmp(p[i].nic, nic) == 0) return i;
    return -1;
}

int findPassengerById(Passenger p[], int n, char id[]) {
    int i;
    for (i = 0; i < n; i++)
        if (strcmp(p[i].passengerId, id) == 0) return i;
    return -1;
}

/* NIC: 12 digits, or 9 digits + V */
static int validNIC(char nic[]) {
    int i, len = strlen(nic);
    if (len == 12) {
        for (i = 0; i < 12; i++) if (!isdigit(nic[i])) return 0;
        return 1;
    }
    if (len == 10) {
        for (i = 0; i < 9; i++) if (!isdigit(nic[i])) return 0;
        return nic[9] == 'V' || nic[9] == 'v';
    }
    return 0;
}

/* Phone: 10 digits starting with 0 */
static int validPhone(char phone[]) {
    int i;
    if (strlen(phone) != 10 || phone[0] != '0') return 0;
    for (i = 0; i < 10; i++) if (!isdigit(phone[i])) return 0;
    return 1;
}

static void readNIC(char nic[]) {
    readText("NIC: ", nic, 13);
    while (!validNIC(nic)) {
        printf("Invalid NIC (12 digits or 9 digits + V).\n");
        readText("NIC: ", nic, 13);
    }
}

/* Register a new passenger with this NIC. Returns new index. */
static int registerNew(char nic[]) {
    Passenger p[MAX_REC];
    int n = loadPassengers(p);

    strcpy(p[n].nic, nic);
    readText("Name : ", p[n].name, sizeof(p[n].name));
    readText("Phone: ", p[n].phone, sizeof(p[n].phone));
    while (!validPhone(p[n].phone)) {
        printf("Invalid phone (10 digits, starts with 0).\n");
        readText("Phone: ", p[n].phone, sizeof(p[n].phone));
    }
    p[n].trips = 0;
    sprintf(p[n].passengerId, "P%03d", (n + 1) % 1000);
    savePassengers(p, n + 1);
    printf("Registered %s (%s)\n", p[n].name, p[n].passengerId);
    return n;
}

/* Used when booking: find passenger by NIC, register if new */
int getPassenger(char idOut[]) {
    Passenger p[MAX_REC];
    char nic[13];
    int n = loadPassengers(p), i;

    readNIC(nic);
    i = findPassengerByNIC(p, n, nic);
    if (i != -1) {
        printf("Passenger: %s (%s), trips: %d\n", p[i].name, p[i].passengerId, p[i].trips);
        strcpy(idOut, p[i].passengerId);
        return 1;
    }
    printf("New passenger - please register.\n");
    i = registerNew(nic);
    n = loadPassengers(p);
    strcpy(idOut, p[i].passengerId);
    return 1;
}

/* Loyalty: 10 or more trips = 5% off */
float discountFor(char passengerId[]) {
    Passenger p[MAX_REC];
    int n = loadPassengers(p), i = findPassengerById(p, n, passengerId);
    if (i != -1 && p[i].trips >= 10) return 0.05f;
    return 0;
}

/* change = +1 after payment, -1 after cancel */
void addPassengerTrip(char passengerId[], int change) {
    Passenger p[MAX_REC];
    int n = loadPassengers(p), i = findPassengerById(p, n, passengerId);
    if (i == -1) return;
    p[i].trips += change;
    if (p[i].trips < 0) p[i].trips = 0;
    savePassengers(p, n);
}

/* ---------------- Menu functions ---------------- */
static void registerPassenger(void) {
    Passenger p[MAX_REC];
    char nic[13];
    int n = loadPassengers(p);
    readNIC(nic);
    if (findPassengerByNIC(p, n, nic) != -1) { printf("NIC already registered.\n"); return; }
    registerNew(nic);
}

static void viewPassengers(void) {
    Passenger p[MAX_REC];
    int n = loadPassengers(p), i;
    printf("\n%-5s %-25s %-13s %-11s %s\n", "ID", "Name", "NIC", "Phone", "Trips");
    for (i = 0; i < n; i++)
        printf("%-5s %-25s %-13s %-11s %d%s\n", p[i].passengerId, p[i].name, p[i].nic,
               p[i].phone, p[i].trips, p[i].trips >= 10 ? "  (5%% discount)" : "");
}

/* Show passenger details and all their bookings */
static void passengerHistory(void) {
    Passenger p[MAX_REC];
    Booking b[MAX_REC];
    char nic[13];
    int n = loadPassengers(p), nb = loadBookings(b), i, k;
    float spent = 0;

    readNIC(nic);
    k = findPassengerByNIC(p, n, nic);
    if (k == -1) { printf("Passenger not found.\n"); return; }
    printf("%s (%s) - phone %s - trips %d\n", p[k].name, p[k].passengerId, p[k].phone, p[k].trips);

    for (i = 0; i < nb; i++)
        if (strcmp(b[i].passengerId, p[k].passengerId) == 0) {
            printf("  %s  trip %s  seat %2d  Rs.%8.2f  %s\n", b[i].bookingId, b[i].tripId,
                   b[i].seat, b[i].amount, b[i].status);
            if (strcmp(b[i].status, "CONFIRMED") == 0) spent += b[i].amount;
        }
    printf("Total spent: Rs. %.2f\n", spent);
    if (p[k].trips < 10) printf("%d more trip(s) for a 5%% discount.\n", 10 - p[k].trips);
}

void passengerMenu(void) {
    int c;
    do {
        printf("\n--- PASSENGERS ---\n");
        printf("1. Register passenger\n2. View all\n3. Passenger history\n0. Back\n");
        c = readInt("Choice: ", 0, 3);
        if (c == 1) registerPassenger();
        else if (c == 2) viewPassengers();
        else if (c == 3) passengerHistory();
    } while (c != 0);
}

/* ---------------- Passenger Kiosk ---------------- */
static void kioskReserve(void) {
    char tripId[6], pid[6];
    Trip t[MAX_REC];
    Route r[MAX_REC];
    Booking b[MAX_REC];
    int nt = loadTrips(t), nr = loadRoutes(r), ti, seat, seatCount, i, idx;
    char from[20], to[20];
    int date, searchTime, matchedTrips[MAX_REC], matchCount = 0;

    printf("\n--- PASSENGER BOOKING FLOW ---\n");
    readText("From: ", from, sizeof(from));
    readText("To: ", to, sizeof(to));
    date = readDate("Date (YYYY-MM-DD): ");
    searchTime = readInt("Time (HHMM, e.g. 600 for 06:00 or 0 for any time): ", 0, 2359);

    /* Find matching routes and trips including time check and active bus check */
    Bus b_kiosk[MAX_REC];
    int nb_kiosk = loadBuses(b_kiosk);
    for (i = 0; i < nt; i++) {
        int k = findRoute(r, nr, t[i].routeId);
        int bi = findBus(b_kiosk, nb_kiosk, t[i].busNo);
        if (k != -1 && bi != -1 && strcmp(b_kiosk[bi].status, "Active") == 0 &&
            strcmp(r[k].from, from) == 0 && strcmp(r[k].to, to) == 0 &&
            t[i].date == date && strcmp(t[i].status, "Scheduled") == 0) {
            if (searchTime == 0 || t[i].time == searchTime) {
                matchedTrips[matchCount++] = i;
            }
        }
    }

    if (matchCount == 0) {
        printf("No scheduled trips found for %s to %s on specified date and time.\n", from, to);
        return;
    }

    printf("\n--- Available Routes / Trips ---\n");
    for (i = 0; i < matchCount; i++) {
        int tidx = matchedTrips[i];
        printf("%d. Trip ID: %s | Time: %02d:%02d | Bus: %s | Fare: Rs. %.2f\n",
               i + 1, t[tidx].tripId, t[tidx].time / 100, t[tidx].time % 100, t[tidx].busNo, t[tidx].fare);
    }

    int choice = readInt("Select Trip (Enter option number): ", 1, matchCount);
    ti = matchedTrips[choice - 1];
    strcpy(tripId, t[ti].tripId);

    /* Display seat map for selected trip */
    showSeatMap(tripId);

    /* Passenger Info */
    printf("\nEnter Passenger Details:\n");
    getPassenger(pid);

    seatCount = readInt("Number of seats to book: ", 1, MAX_SEATS);
    
    for (i = 0; i < seatCount; i++) {
        printf("\n[Seat %d of %d]\n", i + 1, seatCount);
        seat = readInt("Enter seat number to book: ", 1, 60);
        
        idx = makeBooking(tripId, pid, seat, "CONFIRMED", "KIOSK");
        while (idx == -1) {
            printf("Seat %d is already booked or invalid! Please choose another seat.\n", seat);
            seat = readInt("Enter seat number to book: ", 1, 60);
            idx = makeBooking(tripId, pid, seat, "CONFIRMED", "KIOSK");
        }
        
        loadBookings(b);
        addPayment(b[idx].bookingId, b[idx].amount, "PAY");
        addPassengerTrip(pid, 1);
        printf("Seat %d successfully booked! Booking ID: %s\n", seat, b[idx].bookingId);
        printTicket(b[idx].bookingId);
    }

    printf("\n========================================\n");
    printf("All requested seats successfully booked!\n");
    printf("========================================\n");
}

void kioskMenu(void) {
    int c;
    char id[6];
    do {
        printf("\n--- PASSENGER KIOSK ---\n");
        printf("1. Search & Book Seat\n2. Check my booking\n0. Exit\n");
        c = readInt("Choice: ", 0, 2);
        if (c == 1) kioskReserve();
        else if (c == 2) { readText("Booking ID: ", id, sizeof(id)); printTicket(id); }
    } while (c != 0);
}
