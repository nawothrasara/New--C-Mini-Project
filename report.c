/*
 * report.c - Member 5 : Payments & Reports
 * File: payments.txt  format: bookingId|amount|type|clerk|date
 *
 * - Records every payment and refund
 * - Daily income report
 * - Trip occupancy (% of seats sold)
 * - Route income ranking (sorted)
 * - Monthly summary, saved to a text file
 */
#include "common.h"

static int loadPayments(Payment p[]) {
    FILE *fp = fopen(PAYMENT_FILE, "r");
    int n = 0;
    if (fp == NULL) return 0;
    while (n < MAX_REC * 2 && fscanf(fp, " %5[^|]|%f|%7[^|]|%19[^|]|%d", p[n].bookingId,
                                     &p[n].amount, p[n].type, p[n].clerk, &p[n].date) == 5)
        n++;
    fclose(fp);
    return n;
}

/* Add one record to the end of payments.txt */
void addPayment(char bookingId[], float amount, char type[]) {
    FILE *fp = fopen(PAYMENT_FILE, "a");
    if (fp == NULL) return;
    fprintf(fp, "%s|%.2f|%s|%s|%d\n", bookingId, amount, type,
            strlen(currentUser.username) > 0 ? currentUser.username : "SYSTEM", today());
    fclose(fp);
}

static void dailyReport(void) {
    Payment p[MAX_REC * 2];
    int n = loadPayments(p), date, i, tickets = 0;
    float income = 0, refunds = 0;

    date = readDate("Date (YYYY-MM-DD): ");
    for (i = 0; i < n; i++) {
        if (p[i].date != date) continue;
        if (p[i].amount > 0) { income += p[i].amount; tickets++; }
        else refunds -= p[i].amount;
    }
    printf("\nTickets sold : %d\n", tickets);
    printf("Income       : Rs. %.2f\n", income);
    printf("Refunds      : Rs. %.2f\n", refunds);
    printf("Net income   : Rs. %.2f\n", income - refunds);
}

/* Seats sold and income for one trip */
static void tripStats(char tripId[], int *sold, float *income) {
    Booking b[MAX_REC];
    int n = loadBookings(b), i;
    *sold = 0;
    *income = 0;
    for (i = 0; i < n; i++)
        if (strcmp(b[i].tripId, tripId) == 0 && strcmp(b[i].status, "CONFIRMED") == 0) {
            (*sold)++;
            *income += b[i].amount;
        }
}

static void occupancyReport(void) {
    Trip t[MAX_REC];
    Bus b[MAX_REC];
    int nt = loadTrips(t), nb = loadBuses(b), i, k, sold, count = 0;
    float income, percent, total = 0;

    printf("\n%-5s %-11s %-9s %5s %5s %7s %10s\n", "Trip", "Date", "Bus", "Sold", "Seats", "Full%", "Income");
    for (i = 0; i < nt; i++) {
        if (strcmp(t[i].status, "Cancelled") == 0) continue;
        k = findBus(b, nb, t[i].busNo);
        if (k == -1) continue;
        tripStats(t[i].tripId, &sold, &income);
        percent = sold * 100.0f / b[k].seats;
        printf("%-5s ", t[i].tripId); printDate(t[i].date);
        printf(" %-9s %5d %5d %6.1f%% %10.2f%s\n", t[i].busNo, sold, b[k].seats, percent,
               income, percent < 40 ? "  LOW" : "");
        total += percent;
        count++;
    }
    if (count > 0) printf("Average occupancy: %.1f%%\n", total / count);
}

/* Income per route, highest first. out = screen (stdout) or a file. */
static void routeRanking(FILE *out, int month) {
    Route r[MAX_REC], tempR;
    Trip t[MAX_REC];
    float income[MAX_REC] = {0}, tempF, inc;
    int nr = loadRoutes(r), nt = loadTrips(t), i, j, sold;

    for (i = 0; i < nt; i++) {
        if (month != 0 && t[i].date / 100 != month) continue;
        j = findRoute(r, nr, t[i].routeId);
        if (j == -1) continue;
        tripStats(t[i].tripId, &sold, &inc);
        income[j] += inc;
    }
    for (i = 0; i < nr - 1; i++)                 /* bubble sort */
        for (j = 0; j < nr - 1 - i; j++)
            if (income[j] < income[j + 1]) {
                tempF = income[j]; income[j] = income[j + 1]; income[j + 1] = tempF;
                tempR = r[j]; r[j] = r[j + 1]; r[j + 1] = tempR;
            }

    fprintf(out, "\nRoute income ranking:\n");
    for (i = 0; i < nr; i++)
        fprintf(out, "%d. %-5s %-12s -> %-12s Rs. %10.2f\n", i + 1, r[i].routeId,
                r[i].from, r[i].to, income[i]);
}

/* month = YYYYMM */
static void monthlySummary(FILE *out, int month) {
    Payment p[MAX_REC * 2];
    int n = loadPayments(p), i, tickets = 0, refundsCount = 0;
    float income = 0, refunds = 0;

    for (i = 0; i < n; i++) {
        if (p[i].date / 100 != month) continue;
        if (p[i].amount > 0) { income += p[i].amount; tickets++; }
        else { refunds -= p[i].amount; refundsCount++; }
    }
    fprintf(out, "===== MONTHLY REPORT %d-%02d =====\n", month / 100, month % 100);
    fprintf(out, "Tickets sold    : %d\n", tickets);
    fprintf(out, "Cancellations   : %d\n", refundsCount);
    fprintf(out, "Income          : Rs. %.2f\n", income);
    fprintf(out, "Refunds         : Rs. %.2f\n", refunds);
    fprintf(out, "Net income      : Rs. %.2f\n", income - refunds);
    if (tickets > 0) fprintf(out, "Average ticket  : Rs. %.2f\n", income / tickets);
    routeRanking(out, month);
}

static void monthlyReport(void) {
    int month = readInt("Month (YYYYMM, e.g. 202610): ", 202401, 210012);
    char fileName[40];
    FILE *fp;

    monthlySummary(stdout, month);
    if (readInt("\nSave to file? (1-Yes 0-No): ", 0, 1) == 1) {
        sprintf(fileName, "report_%d.txt", month);
        fp = fopen(fileName, "w");
        if (fp == NULL) { printf("Cannot create file.\n"); return; }
        monthlySummary(fp, month);
        fclose(fp);
        printf("Saved to %s\n", fileName);
    }
}

void reportMenu(void) {
    int c;
    do {
        printf("\n--- REPORTS ---\n");
        printf("1. Daily income\n2. Trip occupancy\n3. Route ranking\n4. Monthly report\n0. Back\n");
        c = readInt("Choice: ", 0, 4);
        if (c == 1) dailyReport();
        else if (c == 2) occupancyReport();
        else if (c == 3) routeRanking(stdout, 0);
        else if (c == 4) monthlyReport();
    } while (c != 0);
}
