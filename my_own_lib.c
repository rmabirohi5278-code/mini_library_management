#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

FILE *logf, *binf, *studf;

struct book {
    int bookId;
    char bookName[40];
    char authorName[40];
    int copies;
    char booksoundex[5];
    char authorsoundex[5];
};

struct student {
    int studId;
    char currentBook[40];
    char issueDate[15];
    char returnDate[15];
    int totalCount;
};

char getSoundex(char c) {
    c = toupper(c);
    switch(c) {
        case 'B': case 'F': case 'P': case 'V': return '1';
        case 'C': case 'G': case 'J': case 'K':
        case 'Q': case 'S': case 'X': case 'Z': return '2';
        case 'D': case 'T': return '3';
        case 'L':            return '4';
        case 'M': case 'N': return '5';
        case 'R':            return '6';
        default:             return 0;
    }
}

void soundex(const char *name, char *result) {
    if (name == NULL || strlen(name) == 0) {
        strcpy(result, "0000");
        return;
    }
    result[0] = toupper(name[0]);
    int index = 1;
    char prevcode = getSoundex(name[0]);
    for (int i = 1; name[i] != '\0' && index < 4; i++) {
        char code = getSoundex(toupper(name[i]));
        if (code != 0 && code != prevcode) {
            result[index++] = code;
        }
        prevcode = code;
    }
    while (index < 4) {
        result[index++] = '0';
    }
    result[4] = '\0';
}

int SoundexMatch(const char *name1, const char *name2) {
    char code1[5], code2[5];
    soundex(name1, code1);
    soundex(name2, code2);
    return strcmp(code1, code2) == 0;
}

int isDup(int id) {
    struct book b;
    binf = fopen("bookDetails.dat", "rb");
    if (binf != NULL) {
        while (fread(&b, sizeof(b), 1, binf)) {
            if (b.bookId == id) {
                fclose(binf);
                return 1;
            }
        }
        fclose(binf);
        return 0;
    }
    return 0;
}

int dateDifference(char *date1, char *date2) {
    struct tm t1 = {0}, t2 = {0};
    sscanf(date1, "%d/%d/%d", &t1.tm_mday, &t1.tm_mon, &t1.tm_year);
    sscanf(date2, "%d/%d/%d", &t2.tm_mday, &t2.tm_mon, &t2.tm_year);
    t1.tm_mon -= 1;
    t1.tm_year -= 1900;
    t2.tm_mon -= 1;
    t2.tm_year -= 1900;
    time_t time1 = mktime(&t1);
    time_t time2 = mktime(&t2);
    return (int)(difftime(time2, time1) / 86400);
}

void findNearestReturn(char *bookName) {
    struct student stud;
    char nearestDate[15] = "";
    int minDays = 99999;
    int found = 0;

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char today[15];
    sprintf(today, "%02d/%02d/%04d",
        tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900);

    studf = fopen("studentfile.txt", "r");
    if (studf != NULL) {
        while (fscanf(studf, "%d %s %s %s",
            &stud.studId,
            stud.currentBook,
            stud.issueDate,
            stud.returnDate) == 4)
        {
            if (strcmp(stud.currentBook, bookName) == 0) {
                int days = dateDifference(today, stud.returnDate);
                if (days > 0 && days < minDays) {
                    minDays = days;
                    strcpy(nearestDate, stud.returnDate);
                    found = 1;
                }
            }
        }
        fclose(studf);
    }

    if (found) {
        printf("\nAll copies are exhausted!");
        printf("\nExpected return date  : %s", nearestDate);
        printf("\nBook may be available within %d days!", minDays);
    } else {
        printf("\nNo return records found!");
    }
}

int findingTarget(char *tar, struct book *found) {
    struct book b;
    char tarsoundex[5];
    soundex(tar, tarsoundex);
    binf = fopen("bookDetails.dat", "rb");
    if (binf != NULL) {
        while (fread(&b, sizeof(b), 1, binf)) {
            if (strcmp(tarsoundex, b.booksoundex) == 0 ||
                strcmp(tarsoundex, b.authorsoundex) == 0) {
                *found = b;
                fclose(binf);
                return 1;
            }
        }
        fclose(binf);
    }
    return 0;
}

void updateCopies(struct book *b) {
    struct book temp;
    binf = fopen("bookDetails.dat", "rb+");
    if (binf != NULL) {
        while (fread(&temp, sizeof(temp), 1, binf)) {
            if (temp.bookId == b->bookId) {
                fseek(binf, -(long)sizeof(temp), SEEK_CUR);
                fwrite(b, sizeof(*b), 1, binf);
                break;
            }
        }
        fclose(binf);
    }
}

void manageRecord(char *msg) {
    logf = fopen("record.txt", "a+");
    if (logf != NULL) {
        fprintf(logf, "%s\n", msg);
        fclose(logf);
    }
}

int checkOverdue(int studId) {
    struct student stud;
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char today[15];
    sprintf(today, "%02d/%02d/%04d",
        tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900);

    studf = fopen("studentfile.txt", "r");
    if (studf != NULL) {
        while (fscanf(studf, "%d %s %s %s",
            &stud.studId,
            stud.currentBook,
            stud.issueDate,
            stud.returnDate) == 4)
        {
            if (stud.studId == studId) {
                int diff = dateDifference(stud.returnDate, today);
                if (diff > 0) {
                    printf("\nWarning: You have an overdue book!");
                    printf("\n  Book      : %s", stud.currentBook);
                    printf("\n  Due Date  : %s", stud.returnDate);
                    printf("\n  Overdue by: %d days!", diff);
                    fclose(studf);
                    return 1;
                }
            }
        }
        fclose(studf);
    }
    return 0;
}

void returnBook() {
    int studId;
    struct student stud;
    int found = 0;

    printf("\nEnter your Student ID: ");
    scanf("%d", &studId);

    studf = fopen("studentfile.txt", "r");
    FILE *tempf = fopen("tempfile.txt", "w");

    if (studf == NULL) {
        printf("\nNo issued records found!");
        return;
    }

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char today[15];
    sprintf(today, "%02d/%02d/%04d",
        tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900);

    while (fscanf(studf, "%d %s %s %s",
        &stud.studId,
        stud.currentBook,
        stud.issueDate,
        stud.returnDate) == 4)
    {
        if (stud.studId == studId && !found) {
            found = 1;
            int diff = dateDifference(stud.returnDate, today);

            printf("\n--- Return Details ---");
            printf("\nStudent ID  : %d", stud.studId);
            printf("\nBook        : %s", stud.currentBook);
            printf("\nIssued On   : %s", stud.issueDate);
            printf("\nDue Date    : %s", stud.returnDate);
            printf("\nReturned On : %s", today);

            if (diff > 0) {
                printf("\nOverdue by %d days!", diff);
            } else {
                printf("\nReturned on time!");
            }

            struct book b;
            if (findingTarget(stud.currentBook, &b)) {
                b.copies++;
                updateCopies(&b);
            }

            char buffer[150];
            sprintf(buffer, "Book Returned  ID:%-5d  %s  ReturnedOn:%s",
                stud.studId, stud.currentBook, today);
            manageRecord(buffer);
        } else {
            fprintf(tempf, "%d %s %s %s\n",
                stud.studId,
                stud.currentBook,
                stud.issueDate,
                stud.returnDate);
        }
    }

    fclose(studf);
    fclose(tempf);
    remove("studentfile.txt");
    rename("tempfile.txt", "studentfile.txt");

    if (!found) {
        printf("\nNo issued record found for this ID!");
    } else {
        printf("\nBook returned successfully!");
    }
}

void updateBook() {
    int id;
    struct book b;
    int found = 0;

    printf("\nEnter Book ID to update: ");
    scanf("%d", &id);

    binf = fopen("bookDetails.dat", "rb+");
    if (binf == NULL) {
        printf("\nNo records found!");
        return;
    }

    while (fread(&b, sizeof(b), 1, binf)) {
        if (b.bookId == id) {
            found = 1;

            printf("\nCurrent Details:");
            printf("\nTitle  : %s", b.bookName);
            printf("\nAuthor : %s", b.authorName);
            printf("\nCopies : %d", b.copies);

            printf("\n\nEnter new Title (press enter to keep same): ");
            char newName[40];
            scanf(" %[^\n]", newName);
            if (strlen(newName) > 0) {
                strcpy(b.bookName, newName);
                soundex(b.bookName, b.booksoundex);
            }

            printf("Enter new Author (press enter to keep same): ");
            char newAuthor[40];
            scanf(" %[^\n]", newAuthor);
            if (strlen(newAuthor) > 0) {
                strcpy(b.authorName, newAuthor);
                soundex(b.authorName, b.authorsoundex);
            }

            printf("Enter new Copies count: ");
            int newCopies;
            scanf("%d", &newCopies);
            if (newCopies >= 0) {
                b.copies = newCopies;
            }

            fseek(binf, -(long)sizeof(b), SEEK_CUR);
            fwrite(&b, sizeof(b), 1, binf);

            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char buffer[150];
            sprintf(buffer, "Book Updated  ID:%-5d  %02d/%02d/%04d  %02d:%02d:%02d",
                b.bookId,
                tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            manageRecord(buffer);

            printf("\nBook updated successfully!");
            break;
        }
    }

    fclose(binf);

    if (!found) {
        printf("\nBook ID not found!");
    }
}

void issueBook() {
    char booktar[40];
    struct book b;
    printf("\nEnter the required Book name: ");
    scanf(" %[^\n]", booktar);
    if (findingTarget(booktar, &b)) {
        printf("\nYou searched for  : %s", booktar);
        printf("\nMatched Book      : %s", b.bookName);
        printf("\nAuthor            : %s", b.authorName);
        printf("\nAvailable Copies  : %d", b.copies);
        if (b.copies > 0) {
            struct student stud;
            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);

            printf("\nEnter your ID: ");
            scanf("%d", &stud.studId);

            if (checkOverdue(stud.studId)) {
                printf("\nPlease return overdue book before issuing a new one!");
                return;
            }

            strcpy(stud.currentBook, b.bookName);
            sprintf(stud.issueDate, "%02d/%02d/%04d",
                tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900);
            tm_info->tm_mday += 10;
            mktime(tm_info);
            sprintf(stud.returnDate, "%02d/%02d/%04d",
                tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900);

            studf = fopen("studentfile.txt", "a+");
            if (studf != NULL) {
                fprintf(studf, "%d %s %s %s\n",
                    stud.studId,
                    stud.currentBook,
                    stud.issueDate,
                    stud.returnDate);
                fclose(studf);
            }

            b.copies--;
            updateCopies(&b);

            printf("\n%s is issued to Student ID: %d", b.bookName, stud.studId);
            printf("\nIssue Date  : %s", stud.issueDate);
            printf("\nReturn Date : %s", stud.returnDate);

            char buffer[150];
            sprintf(buffer, "Book Issued  ID:%-5d  %02d/%02d/%04d  %02d:%02d:%02d",
                b.bookId,
                tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            manageRecord(buffer);
        } else {
            findNearestReturn(b.bookName);
        }
    } else {
        printf("\nYou searched for '%s' - Book not found!", booktar);
    }
}

void addBook() {
    struct book b;
    printf("Enter Book ID: ");
    scanf("%d", &b.bookId);
    if (isDup(b.bookId))
        printf("\nThe Id is already available");
    else {
        printf("\nEnter Book Title: ");
        scanf(" %[^\n]", b.bookName);
        printf("\nEnter Author Name: ");
        scanf(" %[^\n]", b.authorName);
        printf("\nEnter Number of Copies: ");
        scanf("%d", &b.copies);

        soundex(b.bookName, b.booksoundex);
        soundex(b.authorName, b.authorsoundex);

        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);

        binf = fopen("bookDetails.dat", "ab");
        if (binf != NULL) {
            char buffer[150];
            fwrite(&b, sizeof(b), 1, binf);
            fclose(binf);
            sprintf(buffer, "Book Added   ID:%-5d  %02d/%02d/%04d  %02d:%02d:%02d",
                b.bookId,
                tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            manageRecord(buffer);
        }
    }
}

int main() {
    int choice;
    while (1) {
        printf("\n------Library Menu------\n");
        printf("1. Add Book\n");
        printf("2. Issue Book\n");
        printf("3. Return Book\n");
        printf("4. Search Book\n");
        printf("5. Update Record\n");
        printf("6. Exit\n");
        scanf("%d", &choice);
        switch (choice) {
            case 1: addBook(); break;
            case 2: issueBook(); break;
            case 3: returnBook(); break;
            case 4: {
                char tar[40];
                struct book found;
                printf("Enter book name to search: ");
                scanf(" %[^\n]", tar);
                if (findingTarget(tar, &found)) {
                    printf("\nYou searched for  : %s", tar);
                    printf("\nBook Found!");
                    printf("\nID     : %d", found.bookId);
                    printf("\nTitle  : %s", found.bookName);
                    printf("\nAuthor : %s", found.authorName);
                    printf("\nCopies : %d", found.copies);
                    if (found.copies == 0) {
                        findNearestReturn(found.bookName);
                    }
                } else {
                    printf("\nYou searched for '%s' - Book not found!", tar);
                }
                break;
            }
            case 5: updateBook(); break;
            case 6: exit(0);
            default: printf("Invalid choice!");
        }
    }
    return 0;
}
