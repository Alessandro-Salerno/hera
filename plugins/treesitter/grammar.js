module.exports = grammar({
  name: 'hera',

  extras: $ => [
    /\s/,
    $.comment,
  ],

  rules: {
    source_file: $ => repeat(choice(
      $.entity_declaration,
      $.relation_declaration,
    )),

    comment: $ => token(choice(
      seq('//', /.*/),
      seq(
        '/*',
        /[^*]*\*+([^/*][^*]*\*+)*/,
        '/'
      )
    )),

    entity_declaration: $ => seq(
      'entity',
      field('name', choice($.identifier, $.string)),
      repeat(choice(
        $.specifies_clause,
        'total',
        'exclusive',
        $.alias_clause
      )),
      $.block
    ),

    relation_declaration: $ => seq(
      'relation',
      field('name', choice($.identifier, $.string)),
      optional($.alias_clause),
      $.block
    ),

    specifies_clause: $ => seq(
      'specifies',
      field('parent', choice($.identifier, $.string))
    ),

    alias_clause: $ => seq(
      'alias',
      field('alias', $.identifier)
    ),

    block: $ => seq(
      '{',
      repeat($._block_item),
      '}'
    ),

    _block_item: $ => seq(
      choice(
        $.attribute_definition,
        $.relation_reference
      ),
      ';'
    ),

    attribute_definition: $ => seq(
      'attribute',
      field('name', choice($.identifier, $.string)),
      optional('key'),
      optional('optional')
    ),

    relation_reference: $ => seq(
      'relation',
      field('name', choice($.identifier, $.string)),
      $.cardinality,
      optional('key'),
      optional('optional')
    ),

    cardinality: $ => seq(
      '(',
      field('min', choice($.number, $.identifier, $.string)),
      ',',
      field('max', choice($.number, $.identifier, $.string)),
      ')'
    ),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    string: $ => seq(
      '"',
      repeat(/[^"\n]/),
      '"'
    ),

    number: $ => /\d+/,
  }
});
