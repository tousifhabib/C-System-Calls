/**
 * @file   course.h
 * @brief  Header file for course types in ENGI 8894/9875 assignment 1
 */

#include <stdint.h>


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

/** Opaque course type. */
struct course;

/**
 * Create a new Course.
 *
 * Returns an object with a refcount of 1.
 */
struct course*	course_create(enum subject, uint16_t code);

/** Retrieve a course's subject. */
enum subject	course_subject(const struct course*);

/** Retrieve a course's course code. */
uint16_t	course_code(const struct course*);

/** Increment a course's refcount. */
void		course_hold(struct course*);

/** Decrement a course's refcount (optionally freeing it). */
void		course_release(struct course*);

/** Retrieve the current reference count of a course. */
int		course_refcount(const struct course*);