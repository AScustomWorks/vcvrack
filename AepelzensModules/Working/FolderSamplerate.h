#ifndef FOLDER_H
#define FOLDER_H

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* Opaque data type SRC_STATE. */
typedef struct SRC_STATE_tag SRC_STATE;

/* SRC_DATA is used to pass data to src_simple() and src_process(). */
typedef struct {
	const float *data_in;
	float *data_out;

	long input_frames, output_frames;
	long input_frames_used, output_frames_gen;

	int end_of_input;

	double	src_ratio;
} SRC_DATA;


/*
**	Standard initialisation function : return an anonymous pointer to the
**	internal state of the converter. Choose a converter from the enums below.
**	Error returned in *error.
*/

SRC_STATE* src_new (int converter_type, int channels, int *error);

/*
**	Cleanup all internal allocations.
**	Always returns NULL.
*/

SRC_STATE* src_delete (SRC_STATE *state);

/*
**	Standard processing function.
**	Returns non zero on error.
*/

int src_process (SRC_STATE *state, SRC_DATA *data);

enum {
	SRC_SINC_BEST_QUALITY		= 0,
	SRC_SINC_MEDIUM_QUALITY		= 1,
	SRC_SINC_FASTEST			= 2,
	SRC_ZERO_ORDER_HOLD			= 3,
	SRC_LINEAR					= 4,
};


#ifdef __cplusplus
}		/* extern "C" */
#endif	/* __cplusplus */

#endif	/* FOLDER_H */

