/*
 * reservation.c - Member 3 : Seat Reservation
 * File: bookings.txt  format: bookingId|tripId|passengerId|seat|amount|status|bookedBy
 *
 * - Seat map of a trip
 * - Book seat (clerk) with loyalty discount and payment
 * - Confirm kiosk booking (PENDING -> CONFIRMED after payment)
 * - Cancel booking with refund based on days before travel
 * - Print ticket, passenger list of a trip
 */
#include "common.h"

int loadBookings(Booking b[]) {
    FILE *fp = fopen(BOOKING_FILE, "r");
    int n = 0;
    if (fp == NULL) return 0;
    while (n < MAX_REC && fscanf(fp, " %5[^|]|%5[^|]|%5[^|]|%d|%f|%11[^|]|%19[^\n]",
                                 b[n].bookingId, b[n].tripId, b[n].passengerId, &b[n].seat,
                                 &b[n].amount, b[n].status, b[n].bookedBy) == 7)
        n++;
    fclose(fp);
    return n;
}

void saveBookings(Booking b[], int n) {
    FILE *fp = fopen(BOOKING_FILE, "w");
    int i;
    for (i = 0; i < n; i++)
        fprintf(fp, "%s|%s|%s|%d|%.2f|%s|%s\n", b[i].bookingId, b[i].tripId, b[i].passengerId,
                b[i].seat, b[i].amount, b[i].status, b[i].bookedBy);
    fclose(fp);
}

int findBooking(Booking b[], int n, char id[]) {
    int i;
    for (i = 0; i < n; i++)
        if (strcmp(b[i].bookingId, id) == 0) return i;
    return -1;
}

/* A seat is taken if its booking is PENDING or CONFIRMED */
static int isActive(Booking b) {
    return strcmp(b.status, "CONFIRMED") == 0 || strcmp(b.status, "PENDING") == 0;
}

static int seatTaken(char tripId[], int seat) {
    Booking b[MAX_REC];
    int n = loadBookings(b), i;
    for (i = 0; i < n; i++)
        if (strcmp(b[i].tripId, tripId) == 0 && b[i].seat == seat && isActive(b[i]))
            return 1;
    return 0;
}

int bookedCount(char tripId[]) {
    Booking b[MAX_REC];
    int n = loadBookings(b), i, count = 0;
    for (i = 0; i < n; i++)
        if (strcmp(b[i].tripId, tripId) == 0 && isActive(b[i])) count++;
    return count;
}

/* Number of seats in the bus of a trip (0 if not found) */
static int tripSeats(char tripId[]) {
    Trip t[MAX_REC];
    Bus b[MAX_REC];
    int nt = loadTrips(t), nb = loadBuses(b), ti, bi;
    ti = findTrip(t, nt, tripId);
    if (ti == -1) return 0;
    bi = findBus(b, nb, t[ti].busNo);
    return bi == -1 ? 0 : b[bi].seats;
}

/* Kiosk bookings must be paid before the travel day.
   Unpaid bookings on or after the travel day are cancelled. */
static void cancelUnpaid(void) {
    Booking b[MAX_REC];
    Trip t[MAX_REC];
    int nb = loadBookings(b), nt = loadTrips(t), i, ti, changed = 0;
    for (i = 0; i < nb; i++) {
        if (strcmp(b[i].status, "PENDING") != 0) continue;
        ti = findTrip(t, nt, b[i].tripId);
        if (ti == -1 || t[ti].date <= today()) {
            strcpy(b[i].status, "CANCELLED");
            changed = 1;
        }
    }
    if (changed) saveBookings(b, nb);
}

/* Seat map: 4 seats in a row (2 + aisle + 2)
   [XX] = booked   [PP] = reserved, not paid */
void showSeatMap(char tripId[]) {
    Booking b[MAX_REC];
    int nb = loadBookings(b), total = tripSeats(tripId), seat, i, mark;

    if (total == 0) { printf("Trip not found.\n"); return; }
    printf("\n        [ DRIVER ]\n");
    for (seat = 1; seat <= total; seat++) {
        mark = 0;
        for (i = 0; i < nb; i++)
            if (strcmp(b[i].tripId, tripId) == 0 && b[i].seat == seat) {
                if (strcmp(b[i].status, "CONFIRMED") == 0) mark = 1;
                else if (strcmp(b[i].status, "PENDING") == 0) mark = 2;
            }
        if (mark == 1) printf("[XX]");
        else if (mark == 2) printf("[PP]");
        else printf("[%02d]", seat);

        if (seat % 4 == 2) printf("    ");      /* aisle */
        if (seat % 4 == 0) printf("\n");
    }
    printf("\nXX = booked, PP = not paid | Free seats: %d of %d\n",
           total - bookedCount(tripId), total);
}

/* Creates a booking. Returns the new booking index, or -1 */
int makeBooking(char tripId[], char passengerId[], int seat, char status[], char by[]) {
    Booking b[MAX_REC];
    Trip t[MAX_REC];
    int nb = loadBookings(b), nt = loadTrips(t), ti = findTrip(t, nt, tripId);
    float fare;

    if (ti == -1 || seat < 1 || seat > tripSeats(tripId) || seatTaken(tripId, seat)) return -1;
    fare = t[ti].fare;

    sprintf(b[nb].bookingId, "B%03d", (nb + 1) % 1000);
    strcpy(b[nb].tripId, tripId);
    strcpy(b[nb].passengerId, passengerId);
    b[nb].seat = seat;
    b[nb].amount = fare - fare * discountFor(passengerId);
    strcpy(b[nb].status, status);
    strcpy(b[nb].bookedBy, by);
    saveBookings(b, nb + 1);
    return nb;
}

void printTicket(char bookingId[]) {
    Booking b[MAX_REC];
    Trip t[MAX_REC];
    Route r[MAX_REC];
    Passenger p[MAX_REC];
    int nb = loadBookings(b), nt = loadTrips(t), nr = loadRoutes(r), np = loadPassengers(p);
    int bi, ti, ri, pi;

    bi = findBooking(b, nb, bookingId);
    if (bi == -1) { printf("Booking not found.\n"); return; }
    ti = findTrip(t, nt, b[bi].tripId);
    ri = ti == -1 ? -1 : findRoute(r, nr, t[ti].routeId);
    pi = findPassengerById(p, np, b[bi].passengerId);

    printf("\n+----------------------------------+\n");
    printf("  TICKET %s\n", b[bi].bookingId);
    printf("  Name   : %s\n", pi != -1 ? p[pi].name : "?");
    if (ri != -1) printf("  Route  : %s -> %s\n", r[ri].from, r[ri].to);
    if (ti != -1) {
        printf("  Date   : "); printDate(t[ti].date);
        printf("  %02d:%02d\n", t[ti].time / 100, t[ti].time % 100);
        printf("  Bus    : %s\n", t[ti].busNo);
    }
    printf("  Seat   : %d\n", b[bi].seat);
    printf("  Amount : Rs. %.2f\n", b[bi].amount);
    printf("  Status : %s\n", b[bi].status);
    printf("+----------------------------------+\n");
}

/* ---------------- Menu functions ---------------- */
static int askTrip(char tripId[]) {
    Trip t[MAX_REC];
    int nt = loadTrips(t), i;
    readText("Trip ID: ", tripId, 6);
    i = findTrip(t, nt, tripId);
    if (i == -1 || strcmp(t[i].status, "Scheduled") != 0 || t[i].date < today()) {
        printf("Trip not available.\n");
        return 0;
    }
    return 1;
}

static void bookSeat(void) {
    char tripId[6], pid[6];
    int seat, idx;
    Booking b[MAX_REC];

    if (!askTrip(tripId)) return;
    showSeatMap(tripId);
    if (bookedCount(tripId) >= tripSeats(tripId)) { printf("Trip is full.\n"); return; }
    if (!getPassenger(pid)) return;

    seat = readInt("Seat number: ", 1, tripSeats(tripId));
    while (seatTaken(tripId, seat)) {
        printf("Seat %d is taken.\n", seat);
        seat = readInt("Seat number: ", 1, tripSeats(tripId));
    }

    idx = makeBooking(tripId, pid, seat, "CONFIRMED", currentUser.username);
    loadBookings(b);
    addPayment(b[idx].bookingId, b[idx].amount, "PAY");
    addPassengerTrip(pid, 1);
    printf("Booking %s confirmed.\n", b[idx].bookingId);
    printTicket(b[idx].bookingId);
}

/* Passenger reserved at kiosk and now pays at the counter */
static void confirmKioskBooking(void) {
    Booking b[MAX_REC];
    char id[6];
    int n = loadBookings(b), i;

    readText("Booking ID: ", id, sizeof(id));
    i = findBooking(b, n, id);
    if (i == -1 || strcmp(b[i].status, "PENDING") != 0) {
        printf("No unpaid booking with this ID.\n");
        return;
    }
    printf("Amount to pay: Rs. %.2f\n", b[i].amount);
    if (readInt("Payment received? (1-Yes 0-No): ", 0, 1) == 0) return;

    strcpy(b[i].status, "CONFIRMED");
    strcpy(b[i].bookedBy, currentUser.username);
    saveBookings(b, n);
    addPayment(id, b[i].amount, "PAY");
    addPassengerTrip(b[i].passengerId, 1);
    printTicket(id);
}

/* Refund: 2 or more days before travel 90%, 1 day 50%, same day 0% */
static void cancelBooking(void) {
    Booking b[MAX_REC];
    Trip t[MAX_REC];
    char id[6];
    int n = loadBookings(b), nt = loadTrips(t), i, ti, days;
    float percent = 0, refund;

    readText("Booking ID: ", id, sizeof(id));
    i = findBooking(b, n, id);
    if (i == -1 || !isActive(b[i])) { printf("No active booking with this ID.\n"); return; }
    ti = findTrip(t, nt, b[i].tripId);
    days = daysBetween(today(), t[ti].date);
    if (days < 0) { printf("Trip is already over.\n"); return; }

    if (strcmp(b[i].status, "CONFIRMED") == 0) {
        if (days >= 2) percent = 0.90f;
        else if (days == 1) percent = 0.50f;
        refund = b[i].amount * percent;
        printf("%d day(s) before travel -> refund %.0f%% = Rs. %.2f\n", days, percent * 100, refund);
        if (readInt("Cancel? (1-Yes 0-No): ", 0, 1) == 0) return;
        if (refund > 0) addPayment(id, -refund, "REFUND");
        addPassengerTrip(b[i].passengerId, -1);
    }
    strcpy(b[i].status, "CANCELLED");
    saveBookings(b, n);
    printf("Booking %s cancelled. Seat %d is free.\n", id, b[i].seat);
}

static void tripPassengerList(void) {
    Booking b[MAX_REC];
    Passenger p[MAX_REC];
    char tripId[6];
    int nb = loadBookings(b), np = loadPassengers(p), i, k, count = 0;

    readText("Trip ID: ", tripId, sizeof(tripId));
    printf("\n%-5s %-6s %-25s %s\n", "Seat", "ID", "Name", "Status");
    for (i = 0; i < nb; i++)
        if (strcmp(b[i].tripId, tripId) == 0 && isActive(b[i])) {
            k = findPassengerById(p, np, b[i].passengerId);
            printf("%-5d %-6s %-25s %s\n", b[i].seat, b[i].bookingId,
                   k != -1 ? p[k].name : "?", b[i].status);
            count++;
        }
    printf("Passengers: %d\n", count);
}

void reservationMenu(void) {
    int c;
    char id[6];
    cancelUnpaid();
    do {
        printf("\n--- RESERVATIONS ---\n");
        printf("1. Seat map\n2. Book seat\n3. Confirm kiosk booking\n4. Cancel booking\n");
        printf("5. Print ticket\n6. Trip passenger list\n0. Back\n");
        c = readInt("Choice: ", 0, 6);
        if (c == 1) { readText("Trip ID: ", id, sizeof(id)); showSeatMap(id); }
        else if (c == 2) bookSeat();
        else if (c == 3) confirmKioskBooking();
        else if (c == 4) cancelBooking();
        else if (c == 5) { readText("Booking ID: ", id, sizeof(id)); printTicket(id); }
        else if (c == 6) tripPassengerList();
    } while (c != 0);
}
