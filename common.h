/*
 * common.h - Bus Reservation & Transport Management System
 * Shared structures, file names and function prototypes.
 * Every .c file includes this file.
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- Data files ---------- */
#define USER_FILE      "data/users.txt"
#define BUS_FILE       "data/buses.txt"
#define ROUTE_FILE     "data/routes.txt"
#define TRIP_FILE      "data/trips.txt"
#define PASSENGER_FILE "data/passengers.txt"
#define BOOKING_FILE   "data/bookings.txt"
#define PAYMENT_FILE   "data/payments.txt"

/* ---------- Limits ---------- */
#define MAX_REC  500     /* max records in any file */
#define MAX_SEATS 60

/* ---------- Structures ---------- */
typedef struct {
    char username[20];
    char password[20];
    char role[10];          /* ADMIN or CLERK */
} User;

typedef struct {
    char  busNo[10];        /* NB-4521 */
    char  type[10];         /* Normal / AC */
    int   seats;            /* 20 - 60 */
    float mileage;          /* km */
    float lastService;      /* km at last service */
    char  status[10];       /* Active / Repair */
} Bus;

typedef struct {
    char  routeId[6];       /* R001 */
    char  from[20];
    char  to[20];
    float km;
} Route;

typedef struct {
    char  tripId[6];        /* T001 */
    char  routeId[6];
    char  busNo[10];
    int   date;             /* YYYYMMDD */
    int   time;             /* HHMM */
    float fare;
    char  status[12];       /* Scheduled / Cancelled */
} Trip;

typedef struct {
    char passengerId[6];    /* P001 */
    char name[30];
    char nic[13];
    char phone[11];
    int  trips;             /* paid trips, for loyalty discount */
} Passenger;

typedef struct {
    char  bookingId[6];     /* B001 */
    char  tripId[6];
    char  passengerId[6];
    int   seat;
    float amount;
    char  status[12];       /* PENDING / CONFIRMED / CANCELLED */
    char  bookedBy[20];     /* clerk username or KIOSK */
} Booking;

typedef struct {
    char  bookingId[6];
    float amount;           /* + payment, - refund */
    char  type[8];          /* PAY / REFUND */
    char  clerk[20];
    int   date;
} Payment;

/* logged in user (main.c) */
extern User currentUser;

/* ===== main.c : login + helper functions ===== */
int   readInt(char msg[], int min, int max);
float readFloat(char msg[], float min, float max);
void  readText(char msg[], char out[], int size);
int   readDate(char msg[]);
int   today(void);
int   daysBetween(int d1, int d2);
void  printDate(int d);

/* ===== bus.c (Member 1) ===== */
int  loadBuses(Bus b[]);
void saveBuses(Bus b[], int n);
int  findBus(Bus b[], int n, char busNo[]);
void busMenu(void);

/* ===== route.c (Member 2) ===== */
int  loadRoutes(Route r[]);
int  findRoute(Route r[], int n, char routeId[]);
int  loadTrips(Trip t[]);
void saveTrips(Trip t[], int n);
int  findTrip(Trip t[], int n, char tripId[]);
void searchTrips(void);
void routeMenu(void);

/* ===== reservation.c (Member 3) ===== */
int  loadBookings(Booking b[]);
void saveBookings(Booking b[], int n);
int  findBooking(Booking b[], int n, char id[]);
int  bookedCount(char tripId[]);
void showSeatMap(char tripId[]);
int  makeBooking(char tripId[], char passengerId[], int seat, char status[], char by[]);
void printTicket(char bookingId[]);
void reservationMenu(void);

/* ===== passenger.c (Member 4) ===== */
int   loadPassengers(Passenger p[]);
void  savePassengers(Passenger p[], int n);
int   findPassengerByNIC(Passenger p[], int n, char nic[]);
int   findPassengerById(Passenger p[], int n, char id[]);
int   getPassenger(char idOut[]);
float discountFor(char passengerId[]);
void  addPassengerTrip(char passengerId[], int change);
void  passengerMenu(void);
void  kioskMenu(void);

/* ===== report.c (Member 5) ===== */
void addPayment(char bookingId[], float amount, char type[]);
void reportMenu(void);

#endif
