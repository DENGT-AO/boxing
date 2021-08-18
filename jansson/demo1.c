#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include <jansson.h>

int main(int argc, char **argv)
{
    json_t *obj = NULL;
    json_t *group = NULL;
    json_t *root = NULL;
    json_t *value = NULL;
    json_t *list = NULL;
    json_error_t  err;
    void *iter = NULL;
    char *buf = NULL;
    int value1 = 0;
    const char *key;
    int ret = 0;
    int error_flag = 0;

    //decoding
    obj = json_load_file("jsondata.txt", 0, &err);
    if (!obj) {
        fprintf( stderr, "err: on line %d: %s\n", err.line, err.text );
        error_flag = 1;
        goto ERR;
    }
    
    group = json_object_get(obj, "group");
    //assert( group );
    if (!group) {
        fprintf(stderr, "err: json_object_get()\n");
        error_flag = 1;
        goto ERR;
    }
    printf("group:%s\n\n",  json_string_value( group ));


    //encoding
    buf = json_dumps(obj, JSON_INDENT(4));
    if (!buf) {
        fprintf(stderr, "err: json_dumps()\n");
        error_flag = 1;
        goto ERR;
    }
    printf("%s\n\n", buf);

    //packing
    root = json_pack("{s:s, s:i}", "frute", "apple", "weight", 1);
    if (!root) {
        fprintf( stderr, "err: json_pack()\n" );
        error_flag = 1;
        goto ERR;
    }

    //array
    list = json_array();
    //assert( list );
    if (!list) {
        fprintf(stderr, "err: json_pack()\n");
        error_flag = 1;
        goto ERR;
    }
    list = json_pack("[ssb]", "foo", "bar", 1);
    if (!root) {
        fprintf( stderr, "err: json_pack()\n" );
        error_flag = 1;
        goto ERR;
    }
    ret = json_array_set_new(list, 2, json_string("third"));
    if (ret) {
        printf("error: set value error  ret = %d\n", ret);
        error_flag = 1;
        goto ERR;
    }
    printf("the list is %s\n", json_dumps(list, JSON_INDENT(4)));
    printf("the list length is %ld\n",  json_array_size(list));
    ret = json_array_append_new(list, json_string("four"));
    if (ret) {
        fprintf( stderr, "err: json_array_append_new()\n" );
        error_flag = 1;
        goto ERR;
    }
    printf("the list is %s\n", json_dumps(list, JSON_INDENT(4)));
    buf = NULL;
    buf = json_dumps( root, JSON_INDENT(4) );
    if (!buf) {
        fprintf(stderr, "err: json_dumps()\n");
        error_flag = 1;
        goto ERR;
    }
    printf( "%s\n", buf );

    //unpacking
    ret = json_unpack(root, "{s:s, s:i}", "frute", &buf, "weight", &value1);
    if (ret) {
        fprintf( stderr, "err: json_unpack()\n" );
        error_flag = 1;
        goto ERR;
    }
    printf( "%s:%d\n", buf, value1 );

    //iter对对象中元素进行迭代，每一对形成键值对
    iter = json_object_iter( root );
    if (!iter) {
        fprintf(stderr, "err: json_object_iter()\n");
        error_flag = 1;
        goto ERR;
    }
    while ( iter )
    {
        key = json_object_iter_key( iter );
        value = json_object_iter_value( iter );
        switch( json_typeof( value ) )
        {
            case JSON_STRING:
                printf( "%s:%s\n", key, json_string_value(value) );
                break;
            case JSON_INTEGER:
                json_unpack( value, "i", &value1 );
                printf( "%s:%d\n", key, value1);
                break;
        }
        iter = json_object_iter_next( root, iter );
    }


ERR:
    free(buf);
    json_decref(group);
    json_decref(value);
    json_decref(obj);
    json_decref(root);

    return (error_flag ? -1 : 0);
}