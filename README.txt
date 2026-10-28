BUS RESERVATION & TRANSPORT MANAGEMENT SYSTEM  (C, console, Ubuntu)

COMPILE AND RUN (inside this folder)
  gcc *.c -o busys
  ./busys

LOGINS
  Admin : admin  / admin123
  Clerk : nimal  / clerk123
  Clerk : kumari / clerk123
  Passenger kiosk: no login (option 2)

FILES - ONE PER MEMBER
  common.h, main.c  Shared: structures, login, menus, input helpers
  bus.c          Member 1  Bus Management          -> data/buses.txt
  route.c        Member 2  Route & Trip Management -> data/routes.txt, data/trips.txt
  reservation.c  Member 3  Seat Reservation        -> data/bookings.txt
  passenger.c    Member 4  Passengers & Kiosk      -> data/passengers.txt
  report.c       Member 5  Payments & Reports      -> data/payments.txt

RULES
  Fare      : Normal bus Rs.2.50/km, AC bus Rs.5.00/km
  Discount  : 5% for passengers with 10 or more trips
  Refund    : 2+ days before travel 90%, 1 day 50%, same day 0%
  Kiosk     : seat is PENDING until paid at the counter; unpaid on the
              travel day -> cancelled automatically
  Service   : every 10,000 km
  Trips     : one bus can have only one trip per day
