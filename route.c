/*
 * route.c - Member 2 : Route & Trip Management
 * Files: routes.txt  format: routeId|from|to|km
 *        trips.txt   format: tripId|routeId|busNo|date|time|fare|status
 *
 * - Add / view / delete routes
 * - Add trip (fare calculated from distance and bus type,
 *   bus must be Active, same bus cannot run twice on one day)
 * - View trips, search trips, cancel trip (full refund)
 */
#include "common.h"

/* Forward prototype */
static void viewRoutes(void);

/* ---------------- Routes ---------------- */
int loadRoutes(Route r[]) {
    FILE *fp = fopen(ROUTE_FILE, "r");
    int n = 0;
    if (fp == NULL) return 0;
    while (n < MAX_REC && fscanf(fp, " %5[^|]|%19[^|]|%19[^|]|%f", r[n].routeId,
                                 r[n].from, r[n].to, &r[n].km) == 4)
        n++;
    fclose(fp);
    return n;
}

static void saveRoutes(Route r[], int n) {
    FILE *fp = fopen(ROUTE_FILE, "w");
    int i;
    for (i = 0; i < n; i++)
        fprintf(fp, "%s|%s|%s|%.0f\n", r[i].routeId, r[i].from, r[i].to, r[i].km);
    fclose(fp);
}

int findRoute(Route r[], int n, char routeId[]) {
    int i;
    for (i = 0; i < n; i++)
        if (strcmp(r[i].routeId, routeId) == 0) return i;
    return -1;
}

/* Delete route function */
static void deleteRoute(void) {
    Route r[MAX_REC];
    char id[6];
    int n = loadRoutes(r), idx, i;

    if (n == 0) {
        printf("No routes available to delete.\n");
        return;
    }

    viewRoutes();
    readText("Enter Route ID to delete: ", id, sizeof(id));
    idx = findRoute(r, n, id);

    if (idx == -1) {
        printf("Route ID %s not found.\n", id);
        return;
    }

    /* Shift elements to delete */
    for (i = idx; i < n - 1; i++) {
        r[i] = r[i + 1];
    }

    saveRoutes(r, n - 1);
    printf("Route %s deleted successfully.\n", id);
}

/* ---------------- Trips ---------------- */
int loadTrips(Trip t[]) {
    FILE *fp = fopen(TRIP_FILE, "r");
    int n = 0;
    if (fp == NULL) return 0;
    while (n < MAX_REC && fscanf(fp, " %5[^|]|%5[^|]|%9[^|]|%d|%d|%f|%11[^\n]", t[n].tripId,
                                 t[n].routeId, t[n].busNo, &t[n].date, &t[n].time,
                                 &t[n].fare, t[n].status) == 7)
        n++;
    fclose(fp);
    return n;
}

void saveTrips(Trip t[], int n) {
    FILE *fp = fopen(TRIP_FILE, "w");
    int i;
    for (i = 0; i < n; i++)
        fprintf(fp, "%s|%s|%s|%d|%d|%.2f|%s\n", t[i].tripId, t[i].routeId, t[i].busNo,
                t[i].date, t[i].time, t[i].fare, t[i].status);
    fclose(fp);
}

int findTrip(Trip t[], int n, char tripId[]) {
    int i;
    for (i = 0; i < n; i++)
        if (strcmp(t[i].tripId, tripId) == 0) return i;
    return -1;
}

/* Normal bus Rs.2.50 per km, AC bus Rs.5.00 per km */
static float calculateFare(float km, char busType[]) {
    if (strcmp(busType, "AC") == 0) return km * 5.0f;
    return km * 2.5f;
}

static void printTrip(Trip t, Route r[], int nr) {
    int k = findRoute(r, nr, t.routeId);
    int seatsFree;
    Bus b[MAX_REC];
    int nb = loadBuses(b), bi = findBus(b, nb, t.busNo);

    seatsFree = (bi == -1 ? 0 : b[bi].seats) - bookedCount(t.tripId);
    printf("%-5s %-11s %-11s ", t.tripId, k != -1 ? r[k].from : "?", k != -1 ? r[k].to : "?");
    printDate(t.date);
    printf(" %02d:%02d %-9s Rs.%7.2f %3d free  %s\n", t.time / 100, t.time % 100,
           t.busNo, t.fare, seatsFree, t.status);
}

/* ---------------- Menu functions ---------------- */
static void addRoute(void) {
    Route r[MAX_REC], x;
    int n = loadRoutes(r);

    readText("From: ", x.from, sizeof(x.from));
    readText("To  : ", x.to, sizeof(x.to));
    x.km = readFloat("Distance (km): ", 1, 1000);
    sprintf(x.routeId, "R%03d", (n + 1) % 1000);
    r[n] = x;
    saveRoutes(r, n + 1);
    printf("Route %s added.\n", x.routeId);
}

static void viewRoutes(void) {
    Route r[MAX_REC];
    int n = loadRoutes(r), i;
    printf("\n%-5s %-12s %-12s %s\n", "ID", "From", "To", "Km");
    for (i = 0; i < n; i++)
        printf("%-5s %-12s %-12s %.0f\n", r[i].routeId, r[i].from, r[i].to, r[i].km);
}

static void createTrip(void) {
    Trip t[MAX_REC], x;
    Route r[MAX_REC];
    Bus b[MAX_REC];
    int nt = loadTrips(t), nr = loadRoutes(r), nb = loadBuses(b), ri, bi, i;

    viewRoutes();
    readText("Route ID: ", x.routeId, sizeof(x.routeId));
    ri = findRoute(r, nr, x.routeId);
    if (ri == -1) { printf("Route not found.\n"); return; }

    readText("Bus number: ", x.busNo, sizeof(x.busNo));
    bi = findBus(b, nb, x.busNo);
    if (bi == -1 || strcmp(b[bi].status, "Active") != 0) {
        printf("Bus not found or not Active.\n");
        return;
    }

    x.date = readDate("Date (YYYY-MM-DD): ");
    if (x.date < today()) { printf("Date is in the past.\n"); return; }

    /* same bus cannot have two trips on the same day */
    for (i = 0; i < nt; i++)
        if (strcmp(t[i].busNo, x.busNo) == 0 && t[i].date == x.date &&
            strcmp(t[i].status, "Scheduled") == 0) {
            printf("Bus %s already has trip %s on this day.\n", x.busNo, t[i].tripId);
            return;
        }

    x.time = readInt("Time (HHMM, e.g. 830 for 08:30): ", 0, 2359);
    x.fare = calculateFare(r[ri].km, b[bi].type);
    strcpy(x.status, "Scheduled");
    sprintf(x.tripId, "T%03d", (nt + 1) % 1000);

    t[nt] = x;
    saveTrips(t, nt + 1);
    printf("Trip %s added. Fare: Rs. %.2f\n", x.tripId, x.fare);
}

static void viewTrips(void) {
    Trip t[MAX_REC];
    Route r[MAX_REC];
    int nt = loadTrips(t), nr = loadRoutes(r), i;
    for (i = 0; i < nt; i++) printTrip(t[i], r, nr);
}

/* Used by clerk and kiosk */
void searchTrips(void) {
    Trip t[MAX_REC];
    Route r[MAX_REC];
    Bus b[MAX_REC];
    char from[20], to[20];
    int nt = loadTrips(t), nr = loadRoutes(r), nb = loadBuses(b), date, i, k, found = 0;

    readText("From: ", from, sizeof(from));
    readText("To  : ", to, sizeof(to));
    date = readDate("Date (YYYY-MM-DD): ");

    for (i = 0; i < nt; i++) {
        k = findRoute(r, nr, t[i].routeId);
        int bi = findBus(b, nb, t[i].busNo);
        if (k != -1 && bi != -1 && strcmp(b[bi].status, "Active") == 0 &&
            strcmp(r[k].from, from) == 0 && strcmp(r[k].to, to) == 0 &&
            t[i].date == date && strcmp(t[i].status, "Scheduled") == 0) {
            printTrip(t[i], r, nr);
            found++;
        }
    }
    if (found == 0) printf("No trips found.\n");
}

/* Cancel trip: all bookings are cancelled with a full refund */
static void cancelTrip(void) {
    Trip t[MAX_REC];
    Booking b[MAX_REC];
    char id[6];
    int nt = loadTrips(t), nb, i;

    readText("Trip ID: ", id, sizeof(id));
    i = findTrip(t, nt, id);
    if (i == -1) { printf("Trip not found.\n"); return; }
    strcpy(t[i].status, "Cancelled");
    saveTrips(t, nt);

    nb = loadBookings(b);
    for (i = 0; i < nb; i++) {
        if (strcmp(b[i].tripId, id) != 0) continue;
        if (strcmp(b[i].status, "CONFIRMED") == 0) {
            addPayment(b[i].bookingId, -b[i].amount, "REFUND");
            addPassengerTrip(b[i].passengerId, -1);
        }
        strcpy(b[i].status, "CANCELLED");
    }
    saveBookings(b, nb);
    printf("Trip %s cancelled. All passengers refunded.\n", id);
}

void routeMenu(void) {
    int c;
    do {
        printf("\n--- ROUTE & TRIP MANAGEMENT ---\n");
        printf("1. Add route\n2. View routes\n3. Delete route\n4. Create trip\n5. View trips\n6. Search trips\n7. Cancel trip\n0. Back\n");
        c = readInt("Choice: ", 0, 7);
        if (c == 1) addRoute();
        else if (c == 2) viewRoutes();
        else if (c == 3) deleteRoute();
        else if (c == 4) createTrip();
        else if (c == 5) viewTrips();
        else if (c == 6) searchTrips();
        else if (c == 7) cancelTrip();
    } while (c != 0);
}
