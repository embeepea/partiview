#ifndef PLUGINS_H

/* declarations for compiled-in "plugins".
 * Mostly here so we can declare them as plain-C entry points
 * to avoid ambiguity.
 */

#ifdef __cplusplus
extern "C" {
#endif

extern void plugin_init(void);

extern void kira_parti_init(void);
extern void warp_init(void);
extern void parti_ieee_init(void);
extern void parti_model_init(void);
extern void conste_init(void);

extern void pp_spi_init(void);
extern void nethack_init(void);
extern void sixdof_init(void);

#ifdef __cplusplus
};
#endif

#endif /*PLUGINS_H*/
