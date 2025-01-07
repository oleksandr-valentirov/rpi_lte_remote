#include "config.h"

cfg_opt_t config_opts[] = {
    CFG_STR("ip_addr", "", CFGF_NONE),
    CFG_INT("port", 7777, CFGF_NONE),
    CFG_STR("name", "alpha", CFGF_NONE),
    CFG_END()
};


cfg_t *config_parse(const char* path) {
    cfg_t *cfg = cfg_init(config_opts, 0);

    if (cfg_parse(cfg, path) == CFG_PARSE_ERROR) {
        cfg_free(cfg);
        return NULL;
    }

    return cfg;
}
