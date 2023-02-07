#include <stdint.h>
#include <stdlib.h>

/** Opaque course type. */
struct course;
typedef struct course course_t;

/** Course subjects. */
enum subject {
    /* Old-fashioned ENGI courses */
    SUBJ_ENGI,

    /* New ECE departmental courses */
    SUBJ_CIV,
    SUBJ_ECE,
    SUBJ_MECH,
    SUBJ_ONAE,
    SUBJ_PROC,

    /* Other course codes of relevance to Engineering */
    SUBJ_CHEM,
    SUBJ_ENGL,
    SUBJ_MATH,
    SUBJ_PHYS,
};

struct course {
    enum subject subjectOfCourse;
    uint16_t code;
    int refcount;
};

course_t *course_create(enum subject subject, uint16_t code){
    course_t *new_course = (course_t*) malloc(sizeof(struct course*));
    new_course->code = code;
    new_course->subjectOfCourse = subject;
    new_course->refcount = 1;
    return new_course;
};

enum subject course_subject(const course_t *course){
    return course->subjectOfCourse;
}

uint16_t course_code(const course_t *course){
    return course->code;
}

void course_hold(course_t *course){
    course->refcount++;
}

void course_release(course_t *course){
    if(course->refcount>0){
        course->refcount--;
    }
}

int course_refcount(const course_t *course){
    return course->refcount;
}