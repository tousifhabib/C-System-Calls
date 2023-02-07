
/**
 * @file   student.h
 * @brief  Header file for student types in ENGI 8894/9875 assignment 1
 */

//REMOVE-ME
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "course.h"

struct course;

struct student;
typedef struct student student_t;

struct student_id;
typedef struct student_id studentId_t;

struct courseAndGrade;
typedef struct courseAndGrade courseAndGrade_t;

struct student_id {
    uint16_t	sid_year;
    uint32_t	sid_serial;
};

struct courseAndGrade{
    struct course *course;
    uint8_t courseGrade;
};

struct student{
    studentId_t studentId;
    bool gradStudent;
    courseAndGrade_t courseAndGrade[20];
    int courseAndGradeIndex;
};

student_t*  student_create(studentId_t student_id, bool grad_student){
    student_t *new_student = (student_t*) malloc(sizeof(student_t *));
    new_student->studentId.sid_year = student_id.sid_year;
    new_student->studentId.sid_serial = student_id.sid_serial;
    new_student->gradStudent = grad_student;
    new_student->courseAndGradeIndex = 0;
    return new_student;
}

void    student_free(student_t *student){
    for (int i = student->courseAndGradeIndex-1; i >=0 ; --i) {
        course_release(student->courseAndGrade[i].course);
        student->courseAndGradeIndex--;
    }
    free(student);
}

void	student_take(student_t *s, struct course * Course, uint8_t grade){
    s->courseAndGrade[s->courseAndGradeIndex].course = Course;
    s->courseAndGrade[s->courseAndGradeIndex].courseGrade = grade;
    s->courseAndGradeIndex++;
    course_hold(Course);
}


int		student_grade(student_t *student, struct course * Course){

    for (int i = student->courseAndGradeIndex-1; i >= 0 ; --i) {
        if(course_subject(student->courseAndGrade[i].course) == course_subject(Course) &&
                course_code(student->courseAndGrade[i].course) == course_code(Course)) {
            return student->courseAndGrade[i].courseGrade;
        }
    }
    return -1;
}

double  student_passed_average(student_t *student){
    uint8_t totalGrade = 0;
    int totalPassed = 0;
    if(student->courseAndGradeIndex<1){
        return 0;
    }
    if(student->gradStudent==false){
        for (int i = 0; i < student->courseAndGradeIndex; ++i) {
            if(student->courseAndGrade[i].courseGrade >= 50){
                totalGrade += student->courseAndGrade[i].courseGrade;
                totalPassed++;
            }
        }
        if(totalPassed<1){
            return 0;
        }
        double average = (double)((double)totalGrade/(double)totalPassed);
        return average;
    }else if(student->gradStudent==true){
        for (int i = 0; i < student->courseAndGradeIndex; ++i) {
            if(student->courseAndGrade[i].courseGrade >= 65){
                totalGrade += student->courseAndGrade[i].courseGrade;
                totalPassed++;
            }
        }
        if(totalPassed<1){
            return 0;
        }
        double average = (double)((double)totalGrade/(double)totalPassed);
        return average;
    }
    return 0;
}

bool student_promotable(student_t *student){
    int failCount = 0;
    int totalGrade = 0;
    int totalPassed = 0;
    if(student->gradStudent==true){
        for (int i = 0; i < student->courseAndGradeIndex; ++i) {
            if(student->courseAndGrade[i].courseGrade >= 65){
                totalGrade += student->courseAndGrade[i].courseGrade;
                totalPassed++;
                } else {
                failCount++;
            }
        }
        if(failCount>1){
            return false;
        } else {
            return true;
        }
    }else if(student->gradStudent==false){
        for (int i = 0; i < student->courseAndGradeIndex; ++i) {
            totalGrade += student->courseAndGrade[i].courseGrade;
            totalPassed++;
        }
        double average = (double)((double)totalGrade/(double)student->courseAndGradeIndex);
        if (average>=60){
            return true;
        } else {
            return false;
        }
    }
}