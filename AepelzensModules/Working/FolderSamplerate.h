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
**	Set a new SRC ratio. This allows step responses
**	in the conversion ratio.
**	Returns non zero on error.
*/

int src_set_ratio (SRC_STATE *state, double new_ratio);

/*
**	Get the current channel count.
**	Returns negative on error, positive channel count otherwise
*/

int src_get_channels (SRC_STATE *state);

/*
**	Reset the internal SRC state.
**	Does not modify the quality settings.
**	Does not free any memory allocations.
**	Returns non zero on error.
*/

int src_reset (SRC_STATE *state);

/*
** Return TRUE if ratio is a valid conversion ratio, FALSE
** otherwise.
*/

int src_is_valid_ratio (double ratio);

/*
**	Return an error number.
*/

int src_error (SRC_STATE *state);

/*
**	Convert the error number into a string.
*/
	
const char* src_strerror (int error);

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

