#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>


struct Book {
    int id;
    char title[50];
    char author[50];
    int copies;
    int issued;
};

struct student{
    int rollno;
    char currentBook[3][50];
    int currentnumber;
    char issueDate[3][20];
    char returnDate[3][20];
};

FILE *fp, *flog,*studf;


//printing in log file

void textmsg(char *msg) {
    flog = fopen("activity.txt", "a");
    if (flog != NULL) {
        fprintf(flog, "%s\n", msg);
        fclose(flog);
    }
}

//Check Duplicate

int duplicate(int id) {
    struct Book b;
    fp = fopen("library.bin", "rb");
    if (fp == NULL)
        return 0;
    while (fread(&b, sizeof(b), 1, fp)) {
        if (b.id == id) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void findNearestReturn(char *bookTitle) {
    struct student stud;
    char nearestDate[20] = "";
    int minDays = 99999;
    int found = 0;
    time_t now = time(NULL);

    studf = fopen("student.bin", "rb");
    if (studf == NULL) {
        printf("All copies are exhausted! No return data available.\n");
        return;
    }

    while (fread(&stud, sizeof(stud), 1, studf)) {
        for (int i = 0; i < stud.currentnumber; i++) {
            if (strcmp(stud.currentBook[i], bookTitle) == 0) {
                struct tm ret = {0};
                sscanf(stud.returnDate[i], "%d-%d-%d",
                    &ret.tm_mday, &ret.tm_mon, &ret.tm_year);
                ret.tm_mon -= 1;
                ret.tm_year -= 1900;
                time_t retTime = mktime(&ret);
                int days = (int)(difftime(retTime, now) / 86400);
                if (days >= 0 && days < minDays) {
                    minDays = days;
                    strcpy(nearestDate, stud.returnDate[i]);
                    found = 1;
                }
            }
        }
    }
    fclose(studf);

    if (found) {
        printf("All copies are exhausted!\n");
        printf("Availability: The book may be available on %s\n", nearestDate);
    } else {
        printf("All copies are exhausted! No return date available.\n");
    }
}

//1.Add book
int addbook() {
    struct Book b;
    time_t t;
    struct tm *tm_info;
    char time_str[30];
    char libmsg[100];
    printf("\n------ADDING BOOKS------\n");
    printf("Enter ID: ");
    scanf("%d", &b.id);
    if (duplicate(b.id)) {
        printf("\nID already exists.\n");
        return 1;
    }
    b.issued=0;
    printf("Enter Title: ");
    scanf(" %[^\n]", b.title);
    printf("Enter Author Name: ");
    scanf(" %[^\n]", b.author);
    printf("Enter Number of Copies: ");
    scanf("%d", &b.copies);
    fp = fopen("library.bin", "ab");
    fwrite(&b, sizeof(b), 1, fp);
    fclose(fp);
    printf("Book added successfully.\n");
    time(&t);
    tm_info = localtime(&t);
    strftime(time_str, sizeof(time_str),"%d-%m-%Y %H:%M:%S", tm_info);
    sprintf(libmsg,"Added Book ID %d at %s",b.id,time_str);
    textmsg(libmsg);
    return 0;
}


//2.ISSUE BOOK

void issue() {
    int id;
    int rollno;
    struct Book b;
    struct student s;
    struct student newstud;
    char time_str[30];
    int studentFound = 0;

    printf("\n------ISSUING BOOKS------\n");
    printf("Enter your Roll Number: ");
    scanf("%d", &rollno);
    printf("\nEnter the Book ID: ");
    scanf("%d", &id);

    studf = fopen("student.bin", "rb");
    if (studf != NULL) {
        while (fread(&s, sizeof(s), 1, studf)) {
            if (s.rollno == rollno) {
                studentFound = 1;
                if (s.currentnumber >= 3) {
                    printf("Sorry! You cannot get more than 3 books at a time!\n");
                    fclose(studf);
                    return;
                }
                break;
            }
        }
        fclose(studf);
    }

    fp = fopen("library.bin", "rb+");
    if (fp == NULL) {
        printf("No records found!\n");
        return;
    }

    while (fread(&b, sizeof(b), 1, fp)) {
        if (b.id == id) {
            if (b.copies > 0) {
                b.copies--;
                b.issued++;
                time_t t;
                struct tm *tm_info;
                time(&t);
                tm_info = localtime(&t);
                strftime(time_str, sizeof(time_str),"%d-%m-%Y %H:%M:%S", tm_info);
                fseek(fp, -(long)sizeof(b), SEEK_CUR);
                fwrite(&b, sizeof(b), 1, fp);
                fclose(fp);

                if (studentFound) {
                    studf = fopen("student.bin", "rb+");
                    while (fread(&s, sizeof(s), 1, studf)) {
                        if (s.rollno == rollno) {
                            int idx = s.currentnumber;
                            strcpy(s.currentBook[idx], b.title);
                            strftime(s.issueDate[idx], sizeof(s.issueDate[idx]), "%d-%m-%Y", tm_info);
                            tm_info->tm_mday += 10;
                            mktime(tm_info);
                            strftime(s.returnDate[idx], sizeof(s.returnDate[idx]), "%d-%m-%Y", tm_info);
                            s.currentnumber++;
                            fseek(studf, -(long)sizeof(s), SEEK_CUR);
                            fwrite(&s, sizeof(s), 1, studf);
                            break;
                        }
                    }
                    fclose(studf);
                } else {
                    memset(&newstud, 0, sizeof(newstud));
                    newstud.rollno = rollno;
                    strcpy(newstud.currentBook[0], b.title);
                    strftime(newstud.issueDate[0], sizeof(newstud.issueDate[0]), "%d-%m-%Y", tm_info);
                    tm_info->tm_mday += 10;
                    mktime(tm_info);
                    strftime(newstud.returnDate[0], sizeof(newstud.returnDate[0]), "%d-%m-%Y", tm_info);
                    newstud.currentnumber = 1;
                    studf = fopen("student.bin", "ab");
                    fwrite(&newstud, sizeof(newstud), 1, studf);
                    fclose(studf);
                }

                printf("Book issued successfully!\n");
                char logmsg[100];
                sprintf(logmsg,"Issued Book ID %d at %s",id,time_str);
                textmsg(logmsg);
            } else {
                fclose(fp);
                findNearestReturn(b.title);
            }
            return;
        }
    }
    printf("Book not found!\n");
    fclose(fp);
}

//3.Return Book
void returning()
{
    int id;
    int rollno;
    struct Book b;
    struct student s;
    time_t t;
    struct tm *tm_info;
    char time_str[30];
    int studentHasBook = 0;
    printf("\n------RETURNING BOOKS------\n");
    printf("Enter your Roll Number: ");
    scanf("%d", &rollno);
    printf("\nEnter ID: ");
    scanf("%d", &id);
    fp = fopen("library.bin", "rb+");
    if (fp == NULL)
    {
        printf("Unable to open file!\n");
        return;
    }
    while (fread(&b, sizeof(b), 1, fp))
    {
        if (b.id == id)
        {
            studf = fopen("student.bin", "rb");
            if (studf != NULL) {
                while (fread(&s, sizeof(s), 1, studf)) {
                    if (s.rollno == rollno) {
                        for (int i = 0; i < s.currentnumber; i++) {
                            if (strcmp(s.currentBook[i], b.title) == 0) {
                                studentHasBook = 1;
                                break;
                            }
                        }
                        break;
                    }
                }
                fclose(studf);
            }
            if (!studentHasBook) {
                printf("This book was not issued to Roll Number %d!\n", rollno);
                fclose(fp);
                return;
            }
            if (b.issued > 0)
            {
                b.copies++;
                b.issued--;
                time(&t);
                tm_info = localtime(&t);
                strftime(time_str, sizeof(time_str),"%d-%m-%Y %H:%M:%S", tm_info);
                fseek(fp, -(long)sizeof(b), SEEK_CUR);
                fwrite(&b, sizeof(b), 1, fp);
                fclose(fp);
                studf = fopen("student.bin", "rb+");
                if (studf != NULL) {
                    while (fread(&s, sizeof(s), 1, studf)) {
                        if (s.rollno == rollno) {
                            for (int i = 0; i < s.currentnumber; i++) {
                                if (strcmp(s.currentBook[i], b.title) == 0) {
                                    for (int j = i; j < s.currentnumber - 1; j++) {
                                        strcpy(s.currentBook[j], s.currentBook[j+1]);
                                        strcpy(s.issueDate[j], s.issueDate[j+1]);
                                        strcpy(s.returnDate[j], s.returnDate[j+1]);
                                    }
                                    s.currentnumber--;
                                    strcpy(s.currentBook[s.currentnumber], "");
                                    strcpy(s.issueDate[s.currentnumber], "");
                                    strcpy(s.returnDate[s.currentnumber], "");
                                    fseek(studf, -(long)sizeof(s), SEEK_CUR);
                                    fwrite(&s, sizeof(s), 1, studf);
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    fclose(studf);
                }
                printf("Book Returned successfully!\n");
                char logmsg[100];
                sprintf(logmsg,"Returned Book ID %d at %s",id, time_str);
                textmsg(logmsg);
            }
            else
            {
                printf("No issued copies to return!\n");
                fclose(fp);
            }
            return;
        }
    }
    printf("Book not found!\n");
    fclose(fp);
}
//(4.1)soundex Algo

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

//4.Searching Book

void searching() {
    struct Book b;
    char tar[50];
    char tarsoundex[5];
    char titlesoundex[5];
    char authorsoundex[5];
    int found = 0;
    printf("\n------SEARCH BOOKS------\n");
    printf("Enter book title or author to search: ");
    scanf(" %[^\n]", tar);

    soundex(tar, tarsoundex);

    fp = fopen("library.bin", "rb");
    if (fp == NULL) {
        printf("No records found!\n");
        return;
    }

    while (fread(&b, sizeof(b), 1, fp)) {
        soundex(b.title, titlesoundex);
        soundex(b.author, authorsoundex);

        if (strcmp(tarsoundex, titlesoundex) == 0 ||
            strcmp(tarsoundex, authorsoundex) == 0) {
            found = 1;
            printf("\nYou searched for : %s", tar);
            printf("\nBook Found!");
            printf("\nID     : %d", b.id);
            printf("\nTitle  : %s", b.title);
            printf("\nAuthor : %s", b.author);
            printf("\nCopies : %d\n", b.copies);
            if (b.copies == 0) {
                fclose(fp);
                findNearestReturn(b.title);
                return;
            }
        }
    }

    fclose(fp);

    if (!found) {
        printf("\nYou searched for '%s' - Book not found!\n", tar);
    }
}

//5.Updating Copies

void updateCopies() {
    int id, newCopies;
    int found = 0;
    struct Book b;
    printf("\n--- UPDATE BOOK Detials ---\n");
    printf("Enter Book ID to update: ");
    scanf("%d", &id);
    fp = fopen("library.bin", "rb+");
    if (!fp) {
        printf("No records found! Add books first.\n");
        return;
    }
    while (fread(&b, sizeof(b), 1, fp)) {
        if (b.id == id) {
            printf("\nFound Book!\n");
            printf("Title         : %s\n", b.title);
            printf("Author        : %s\n", b.author);
            printf("Current Copies: %d\n", b.copies);
            printf("\nEnter new copies count: ");
            scanf("%d", &newCopies);
            if (newCopies < 0) {
                printf("Copies cannot be negative!\n");
                fclose(fp);
                return;
            }
            b.copies = newCopies;
            time_t t;
            struct tm *tm_info;
            time(&t);
            tm_info = localtime(&t);
            char time_str[30];
            strftime(time_str, sizeof(time_str),"%d-%m-%Y %H:%M:%S", tm_info);
            fseek(fp, -(long)sizeof(b), SEEK_CUR);
            fwrite(&b, sizeof(b), 1, fp);
            printf("Copies updated successfully!\n");
            char logmsg[100];
            sprintf(logmsg,"Updated copies of Book ID %d to %d at %s",id,newCopies,time_str);
            textmsg(logmsg);
            found = 1;
            break;
        }
    }
    fclose(fp);
    if (!found)
        printf("Book ID %d not found!\n", id);
}


//6.Deleting Books

void deleteBook() {
    int id;
    int found = 0;
    struct Book b;
    FILE *temp;
    printf("\n--- DELETE BOOK ---\n");
    printf("Enter Book ID to delete: ");
    scanf("%d", &id);
    fp = fopen("library.bin", "rb");
    if (!fp) {
        printf("No records found!\n");
        return;
    }
    temp = fopen("temp.dat", "wb");
    if (temp!=NULL){
        while (fread(&b, sizeof(b), 1, fp)) {
            if (b.id != id) {
                fwrite(&b, sizeof(b), 1, temp);
            } else {
                printf("\nDeleting Book:\n");
                printf("Title  : %s\n", b.title);
                printf("Author : %s\n", b.author);
                printf("Copies : %d\n", b.copies);
                found = 1;
            }
        }
        fclose(temp);
    }
    fclose(fp);
    remove("library.bin");
    rename("temp.dat", "library.bin");
    time_t t;
    struct tm *tm_info;
    time(&t);
    char time_str[30];
    tm_info = localtime(&t);
    strftime(time_str, sizeof(time_str),"%d-%m-%Y %H:%M:%S", tm_info);
    if (found) {
        printf("Book ID %d deleted successfully!\n", id);
        char logmsg[100];
        sprintf(logmsg,"Deleted Book ID %d at %s",id,time_str);
        textmsg(logmsg);
    } else {
        printf("Book ID %d not found!\n", id);
    }
}

//Main function
int main()
{
    while(1){
             printf("--------MENU--------\n");
             printf("1. Add Book\n");
             printf("2. Issue Book\n");
             printf("3. Return Book\n");
             printf("4. Search Book\n");
             printf("5. Update Book Copies\n");
             printf("6. Delete Book\n");
             printf("7. Exit\n");
             int choice;
             printf("Enter your choice: ");
             scanf("%d",&choice);
             switch(choice)
             {
             case 1:
                 addbook();
                 break;
             case 2:
                 issue();
                 break;
             case 3:
                returning();
                break;
             case 4:
                searching();
                break;
             case 5:
                 updateCopies();
                 break;
             case 6:
                deleteBook();
                break;
             case 7:
                exit(1);
             default:
                printf("\nInvalid Choice!");
             }
             printf("\n");
    }
    return 0;
}
