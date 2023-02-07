/**
 * @file   student.h
 * @brief  Header file for student types in ENGI 8894/9875 assignment 1
 */

#include <stdbool.h>
#include <stdint.h>


/* Forward declaration of course type. */
struct course;

/** Opaque type representing a student. */
struct student;

/** A student ID is composed of an application year and a serial number. */
struct student_id {
	uint16_t	sid_year;
	uint32_t	sid_serial;
};

/**
 * Create a new student object.
 *
 * The caller is responsible for freeing the returned memory.
 */
struct student*	student_create(struct student_id, bool grad_student);

/**
 * Release a student object.
 */
void		student_free(struct student*);

/**
 * Note that a student has taken a course.
 *
 * This student will now take a reference to (i.e., increment the refcount of)
 * the course that is passed in.
 */
void		student_take(struct student *s, struct course*, uint8_t grade);

/**
 * Retrieve a student's mark in a course.
 *
 * This will retrieve the most recent grade that a student achieved in a
 * particular course.
 *
 * @returns    a grade, or -1 if the student has not taken the course
 */
int		student_grade(struct student*, struct course*);

/**
 * Determine the average grade in the courses this student has passed.
 *
 * This will average the grades in courses that the student has passed, meaning
 * that they scored at least 50 (if undergraduate) or 65 (if graduate).
 *
 * @returns     the average, or 0 if no courses have been passed
 */
double		student_passed_average(const struct student*);

/**
 * Determine whether or not this student is promotable.
 *
 * (note: this is not how promotion really works... simplified for assignment)
 *
 * For graduate students, determine whether they have passed all of their
 * courses apart from (up to) one.
 *
 * For undergraduate students, determine whether or not the cumulative average
 * of all courses is at least 60%.
 */
bool		student_promotable(const struct student*);